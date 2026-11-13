#ifndef __CX_UUID_NAME_VECTORS_H__
#define __CX_UUID_NAME_VECTORS_H__

// Independent MD5/SHA-1 reference results: RFC DNS namespace followed by
// length copies of ASCII 'a', computed using System.Security.Cryptography.
static const struct { unsigned length; const char *v3, *v5; } name_vectors[] = {
    { 0, "c87ee674-4ddc-3efe-a74e-dfe25da5d7b3", "4ebd0208-8328-5d69-8c44-ec50939c0967" },
    { 39, "96cb729a-b665-38ba-b98f-a35a1d044728", "5824f981-4282-59d4-9716-acb6d741350e" },
    { 40, "13c085b8-0e53-35ed-bd46-f814ae2cd6cf", "39f39c20-db47-5131-8879-62f8f67f9014" },
    { 47, "f41abfa0-01e6-34a5-ad0c-0c9835688c00", "660c273c-8a00-5941-b6f4-8d0afed88966" },
    { 48, "12adee6c-b187-318d-82d2-f934bf55422b", "7280cc42-274a-5c4a-91fc-ae23f853eeb7" },
    { 55, "51ba8747-7b49-377b-9214-fbe92f38fc26", "ab36061e-9983-59b3-a171-f60bda1e5897" },
    { 56, "b32b1590-d3a6-3fd2-9e8a-7d7e9b646ce5", "3fa6c3cd-527c-5c97-b3b3-25a1c8b0e0ba" },
    { 63, "0255a4e2-7803-324e-9ee6-96147b40fc2a", "11a49e01-6620-5323-95d5-c4828b787233" },
    { 64, "3a55c73f-2fc6-3ce3-9219-6b3699fdb604", "efd9d9de-780b-5e83-a3e7-30a408967cf8" },
    { 65, "48c88c67-4fd8-3855-9eb9-068f4f0d0acd", "6f0825db-787f-5a25-98d3-018aafd132de" },
    { 1000, "725a217e-8bab-3652-9725-d0ab6260e34b", "062a6b1a-ddc3-5fcc-b238-790846e533d6" },
};

#endif // __CX_UUID_NAME_VECTORS_H__
