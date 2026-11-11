param(
    [ValidateSet('get','restore')][string]$Operation,
    [string]$Target,
    [string]$Sddl
)
$ErrorActionPreference = 'Stop'
if ($Operation -eq 'get') {
    (Get-Acl -LiteralPath $Target).Sddl
} else {
    # Construct the saved ACL without first reading a deliberately denied path.
    $acl = New-Object System.Security.AccessControl.DirectorySecurity
    $acl.SetSecurityDescriptorSddlForm($Sddl)
    Set-Acl -LiteralPath $Target -AclObject $acl
}
