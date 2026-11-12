param(
    [ValidateSet('get','restore','readonly','deny')][string]$Operation,
    [string]$Target,
    [string]$Sddl
)
$ErrorActionPreference = 'Stop'
$sections = [System.Security.AccessControl.AccessControlSections]::Access
if ($Operation -eq 'get') {
    $acl = [System.IO.Directory]::GetAccessControl($Target, $sections)
    $acl.GetSecurityDescriptorSddlForm($sections)
} elseif ($Operation -eq 'restore') {
    # Construct the saved ACL without first reading a deliberately denied path.
    $acl = [System.Security.AccessControl.DirectorySecurity]::new()
    $acl.SetSecurityDescriptorSddlForm($Sddl, $sections)
    [System.IO.Directory]::SetAccessControl($Target, $acl)
} else {
    $sid = [System.Security.Principal.WindowsIdentity]::GetCurrent().User
    if ($Operation -eq 'readonly') {
        $acl = [System.Security.AccessControl.DirectorySecurity]::new()
        $acl.SetSecurityDescriptorSddlForm(('D:P(A;OICI;FRFX;;;' + $sid.Value + ')'), $sections)
    } else {
        $acl = [System.IO.Directory]::GetAccessControl($Target, $sections)
        $inherit = [System.Security.AccessControl.InheritanceFlags]::ContainerInherit -bor
                   [System.Security.AccessControl.InheritanceFlags]::ObjectInherit
        $rule = [System.Security.AccessControl.FileSystemAccessRule]::new(
            $sid, [System.Security.AccessControl.FileSystemRights]::ReadAndExecute,
            $inherit, [System.Security.AccessControl.PropagationFlags]::None,
            [System.Security.AccessControl.AccessControlType]::Deny)
        $acl.AddAccessRule($rule)
    }
    [System.IO.Directory]::SetAccessControl($Target, $acl)
}
