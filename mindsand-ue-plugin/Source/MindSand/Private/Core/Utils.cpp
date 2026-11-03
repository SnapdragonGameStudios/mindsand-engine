// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Utils.h"

namespace Utils
{
FString Serialize(TSharedRef<FJsonObject> Root)
{
    FString OutputString;
    TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
        TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&OutputString);
    FJsonSerializer::Serialize(Root, Writer);
    return OutputString;
}
} // namespace Utils
