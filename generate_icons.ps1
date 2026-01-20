
Add-Type -AssemblyName System.Drawing

$srcPath = "d:\program\C++\QtProject\practice_questions\favicon.png"
$baseDir = "d:\program\C++\QtProject\practice_questions\android\res"

# Map of density to size
$sizes = @{
    "mipmap-mdpi"    = 48
    "mipmap-hdpi"    = 72
    "mipmap-xhdpi"   = 96
    "mipmap-xxhdpi"  = 144
    "mipmap-xxxhdpi" = 192
}

try {
    $srcImg = [System.Drawing.Image]::FromFile($srcPath)
    
    foreach ($name in $sizes.Keys) {
        $size = $sizes[$name]
        $dirPath = Join-Path $baseDir $name
        
        if (!(Test-Path $dirPath)) {
            New-Item -ItemType Directory -Force -Path $dirPath | Out-Null
        }
        
        $destPath = Join-Path $dirPath "ic_launcher.png"
        
        $bmp = New-Object System.Drawing.Bitmap $size, $size
        $graph = [System.Drawing.Graphics]::FromImage($bmp)
        $graph.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $graph.DrawImage($srcImg, 0, 0, $size, $size)
        
        $bmp.Save($destPath, [System.Drawing.Imaging.ImageFormat]::Png)
        
        $graph.Dispose()
        $bmp.Dispose()
        
        Write-Host "Generated $destPath"
    }
    
    $srcImg.Dispose()
    Write-Host "Icon generation complete."
} catch {
    Write-Error "Failed to generate icons: $_"
    exit 1
}
