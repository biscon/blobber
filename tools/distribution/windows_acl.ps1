param(
    [ValidateSet('get','restore')][string]$Operation,
    [string]$Target,
    [string]$Sddl
)
$ErrorActionPreference = 'Stop'
$sections = [System.Security.AccessControl.AccessControlSections]::Access
if ($Operation -eq 'get') {
    $acl = [System.IO.Directory]::GetAccessControl($Target, $sections)
    $acl.GetSecurityDescriptorSddlForm($sections)
} else {
    # Construct the saved ACL without first reading a deliberately denied path.
    $acl = [System.Security.AccessControl.DirectorySecurity]::new()
    $acl.SetSecurityDescriptorSddlForm($Sddl, $sections)
    [System.IO.Directory]::SetAccessControl($Target, $acl)
}
