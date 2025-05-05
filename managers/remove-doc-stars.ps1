# Script to remove lines containing only a * from C/C++ files with Doxygen comments
# save as: remove-doc-stars.ps1

param(
    [Parameter(Mandatory=$true)]
    [string]$Directory
)

# Get all C/C++ header and source files
$files = Get-ChildItem -Path $Directory -Recurse -Include *.h,*.hpp,*.c,*.cpp

foreach ($file in $files) {
    Write-Host "Processing file: $($file.FullName)"
    
    # Read file content
    $content = Get-Content -Path $file.FullName
    
    # Filter out lines that contain only a * (with possible whitespace)
    $newContent = $content | Where-Object { $_ -notmatch '^\s*\*\s*$' }
    
    # Write back to file if changes were made
    if ($content.Count -ne $newContent.Count) {
        $newContent | Set-Content -Path $file.FullName
        Write-Host "  Removed $($content.Count - $newContent.Count) star lines"
    } else {
        Write-Host "  No changes needed"
    }
}

Write-Host "Done!"