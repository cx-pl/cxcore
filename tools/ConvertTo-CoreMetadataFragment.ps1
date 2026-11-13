# Metadata fragments share the native-only core descriptor in cxmodule.c.
function ConvertTo-CoreMetadataFragment([string] $Metadata) {
    $pattern = '(?s)#if !defined\(CX_MODULE_METADATA_FORMAT_VERSION\).*?(?=static struct cx_runtime_once __cx_module_once_CXCORE;)'
    if (![regex]::IsMatch($Metadata, $pattern)) {
        throw 'Generated core metadata has no recognizable module descriptor section.'
    }
    return [regex]::Replace($Metadata, $pattern, '')
}
