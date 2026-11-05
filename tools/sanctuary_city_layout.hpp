#pragma once
#include "world/sanctuary_city.hpp"
#include <ostream>
namespace blobber {
// Diagnostic vector map generated from authoritative cells; no texture/GPU dependency.
inline void writeCityLayout(std::ostream& out, const SanctuaryCityReview& city) {
    constexpr int scale = 32, ox = 28, oy = 76;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1160\" height=\"920\" viewBox=\"0 0 1160 920\">"
           "<rect width=\"1160\" height=\"920\" fill=\"#f3eee0\"/>"
           "<g font-family=\"sans-serif\" fill=\"#302e28\"><text x=\"28\" y=\"32\" font-size=\"24\">"
           "Sanctuary — SC02 architectural layout</text><text x=\"28\" y=\"55\" font-size=\"14\">"
           "North ↑ · 24 × 24 cells · reserved dressing is deferred to SC03</text>";
    auto cell = [&](Cell c, const char* color) {
        out << "<rect x=\"" << ox + c.x * scale << "\" y=\"" << oy + c.y * scale
            << "\" width=\"32\" height=\"32\" fill=\"" << color
            << "\" stroke=\"#d3cbb9\" stroke-width=\".5\"/>";
    };
    for (int y = 0; y < city.map.height(); ++y)
        for (int x = 0; x < city.map.width(); ++x)
            cell({x, y}, city.map.solid({x, y}) ? "#9d9485" : "#e9dfcb");
    for (unsigned i = 0; i < city.reservationCount; ++i) {
        const auto& r = city.reservations[i];
        for (unsigned y = 0; y < r.depth; ++y)
            for (unsigned x = 0; x < r.width; ++x)
                cell(r.origin + Cell{int(x), int(y)}, i == 0 ? "#a8c6cf" : i == 1 ? "#d2b07c" : "#b0c29b");
        out << "<text x=\"830\" y=\"" << 570 + i * 23 << "\" font-size=\"14\">" << r.name << " ("
            << r.origin.x << ',' << r.origin.y << ")</text>";
    }
    for (const auto& loop : city.loops) {
        out << "<polyline fill=\"none\" stroke=\"#817143\" stroke-width=\"3\" stroke-dasharray=\"6 5\" "
               "points=\"";
        for (auto p : loop.corners)
            out << ox + p.x * scale + 16 << ',' << oy + p.y * scale + 16 << ' ';
        out << "\"/>";
    }
    auto marker = [&](Cell c, const std::string& label, const char* color) {
        out << "<circle cx=\"" << ox + c.x * scale + 16 << "\" cy=\"" << oy + c.y * scale + 16
            << "\" r=\"12\" fill=\"" << color << "\"/><text x=\"" << ox + c.x * scale + 16 << "\" y=\""
            << oy + c.y * scale + 21 << "\" text-anchor=\"middle\" fill=\"white\" font-size=\"13\">" << label
            << "</text>";
    };
    out << "<text x=\"830\" y=\"95\" font-size=\"18\">Buildings / approaches</text>";
    for (unsigned i = 0; i < city.doorCount; ++i) {
        const auto& d = city.doors[i];
        const auto& b = city.map.definition().buildings[d.placement];
        marker(b.anchor, std::to_string(i + 1), "#675b4f");
        const auto c = d.approach.cell, f = forward(d.approach.heading);
        out << "<path d=\"M " << ox + c.x * scale + 16 << ' ' << oy + c.y * scale + 16 << " l " << f.x * 11
            << ' ' << f.y * 11 << "\" stroke=\"#a14e33\" stroke-width=\"4\"/>"
            << "<text x=\"830\" y=\"" << 125 + i * 26 << "\" font-size=\"15\">" << i + 1 << ". " << d.name
            << "</text>";
    }
    marker(city.spawn.cell, "A", "#42757e");
    out << "<text x=\"830\" y=\"470\" font-size=\"14\">A — arrival (11,8), facing north</text>"
           "<text x=\"830\" y=\"495\" font-size=\"14\">Dashed — connected street loops</text>"
           "<text x=\"830\" y=\"520\" font-size=\"14\">Red — accessible door approaches</text>"
           "<text x=\"830\" y=\"545\" font-size=\"18\">Future dressing bases</text>"
           "<text x=\"28\" y=\"875\" font-size=\"16\">Gate stays closed. Services, interiors and campaign "
           "travel are not enabled.</text>"
           "<text x=\"28\" y=\"901\" font-size=\"14\">Paving around the fountain stays clear; gate approach "
           "is three cells wide. Coordinates start at the northwest corner.</text>"
           "</g></svg>\n";
    if (!out)
        throw std::runtime_error("Cannot write city layout");
}
} // namespace blobber
