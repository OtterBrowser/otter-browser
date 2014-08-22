# Title: PowerShell package build script
# Description: Build packages with installation
# Type: script
# Author: bajasoft <jbajer@gmail.com>
# Version: 0.1

# How to run PowerShell scripts: http://technet.microsoft.com/en-us/library/ee176949.aspx

# Command line parameters
# [-l] - use limited build with specified files (otter-browser.ps1 -l)
# [-o] - set output file name (otter-browser.ps1 -o nameOfFile)

Param(
  [switch]$l,
  [string]$o="Otter-browser"
)

# Global values

$Global:outputPath = ".\output\"
$Global:inputPath= "Z:\otter-browser-inno\input\*"
$Global:7ZipBinaryPath = "C:\Program Files\7-Zip\7z.exe"
$Global:innoBinaryPath = "C:\Program Files\Inno Setup 5\ISCC.exe"
$Global:innoScriptPath = ".\otter-browser.iss"
$Global:fileList = "Z:\otter-browser-inno\input\otter-browser.exe Z:\otter-browser-inno\input\locale\*"

# Main-function
function main 
{   
    $7zipFileName = $Global:outputPath + $o + ".7z"
    $zipFileName = $Global:outputPath + $o + ".zip"
    
    if ($l)
    {
        # Do limited build without installer
        limitedBuild $7zipFileName $zipFileName
    }
    else
    {
        # Do full build
        fullBuild $7zipFileName $zipFileName
    }
}

function fullBuild ($7zipFileName, $zipFileName)
{
    Start-Process -NoNewWindow -Wait $Global:innoBinaryPath $Global:innoScriptPath 
    Start-Process -NoNewWindow -Wait $Global:7ZipBinaryPath -ArgumentList "a", $7zipFileName, $Global:inputPath, "-mmt", "-mx9"
    Start-Process -NoNewWindow -Wait $Global:7ZipBinaryPath -ArgumentList "a", "-tzip", $zipFileName, $Global:inputPath, "-mx5"
}

function limitedBuild ($7zipFileName, $zipFileName)
{
    Start-Process -NoNewWindow -Wait $Global:7ZipBinaryPath -ArgumentList "a", $7zipFileName, $Global:fileList, "-mmt", "-mx9"
    Start-Process -NoNewWindow -Wait $Global:7ZipBinaryPath -ArgumentList "a", "-tzip", $zipFileName, $Global:fileList, "-mx5"
}

# Entry point
main
