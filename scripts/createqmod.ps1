Param(
    [Parameter(Mandatory=$false)]
    [String] $qmodName="",

    [Parameter(Mandatory=$false)]
    [Switch] $help
)

if ($help) {
    Write-Output 'Creates a QMOD from the current template, resolved dependencies, and compiled libraries.'
    Write-Output '-QmodName: optional output name without the .qmod extension'
    exit 0
}

& qpm qmod manifest --offline
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($qmodName) {
    & qpm qmod zip --offline --skip_build ($qmodName + '.qmod')
} else {
    & qpm qmod zip --offline --skip_build
}
exit $LASTEXITCODE
