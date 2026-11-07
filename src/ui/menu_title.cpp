#include "assets/prepared_image.hpp"
#include "ui/art.hpp"
#include <cmath>
#include <stdexcept>
namespace blobber {
namespace {
constexpr double cycleSeconds = 12;
constexpr const char* fragment = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float phaseSeconds;
out vec4 finalColor;
void main() {
    vec4 base = texture(texture0, fragTexCoord) * fragColor * colDiffuse;
    float luminance = dot(base.rgb, vec3(0.2126, 0.7152, 0.0722));
    float bevel = smoothstep(0.13, 0.72, luminance);

    // Integer harmonics keep the small, uneven warmth variation seamless.
    float angle = phaseSeconds * (6.28318530718 / 12.0);
    float torch = sin(angle * 7.0) + 0.45 * sin(angle * 11.0 + fragTexCoord.x * 3.0);
    vec3 color = base.rgb * (vec3(1.0) + vec3(0.016, 0.007, -0.004) * torch * bevel);

    // Two-second diagonal sweep, followed by ten seconds of rest.
    if (phaseSeconds >= 1.0 && phaseSeconds < 3.0) {
        float progress = (phaseSeconds - 1.0) * 0.5;
        float center = mix(-0.20, 1.20, progress);
        float distance = fragTexCoord.x + (fragTexCoord.y - 0.5) * 0.12 - center;
        float shoulder = exp(-pow(distance / 0.055, 2.0));
        float core = exp(-pow(distance / 0.012, 2.0));
        color += bevel * (vec3(1.0, 0.72, 0.30) * shoulder * 0.23
                        + vec3(1.0, 0.93, 0.72) * core * 0.36);
    }
    finalColor = vec4(clamp(color, 0.0, 1.0), base.a);
})";
} // namespace
void UiArt::prepareMenuTitle(const std::filesystem::path& file) {
    if (menuTitle_)
        return;
    OwnedImage image(preparedImage(file));
    if (!image.value.data || image.value.width > 4096 || image.value.height > 2048)
        throw std::runtime_error("Missing or invalid menu title artwork");
    const auto border = GetImageAlphaBorder(image.value, .1f);
    if (border.width <= 0 || border.height <= 0)
        throw std::runtime_error("Empty menu title artwork");
    ImageCrop(&image.value, border);
    if (!image.value.data)
        throw std::runtime_error("Cannot prepare menu title artwork");
    GpuLease<TextureHandle> texture(resources_, resources_.createTexture(image.value));
    SetTextureFilter(resources_.texture(texture.get()), TEXTURE_FILTER_BILINEAR);
    GpuLease<ShaderHandle> shader(resources_, resources_.createShader(nullptr, fragment));
    const int location = GetShaderLocation(resources_.shader(shader.get()), "phaseSeconds");
    if (location < 0)
        throw std::runtime_error("Menu title shader interface unavailable");
    menuTitlePhaseLocation_ = location;
    menuTitle_ = std::move(texture);
    menuTitleShader_ = std::move(shader);
}
void UiArt::menuTitle(UiRect nativeDestination, double elapsedSeconds) const {
    const float phase = std::isfinite(elapsedSeconds) && elapsedSeconds > 0
                            ? float(std::fmod(elapsedSeconds, cycleSeconds))
                            : 0.f;
    const auto& texture = resources_.texture(menuTitle_.get());
    const auto& shader = resources_.shader(menuTitleShader_.get());
    SetShaderValue(shader, menuTitlePhaseLocation_, &phase, SHADER_UNIFORM_FLOAT);
    BeginShaderMode(shader);
    DrawTexturePro(texture, {0, 0, float(texture.width), float(texture.height)},
                   {float(nativeDestination.x), float(nativeDestination.y), float(nativeDestination.width),
                    float(nativeDestination.height)},
                   {0, 0}, 0, WHITE);
    EndShaderMode();
}
} // namespace blobber
