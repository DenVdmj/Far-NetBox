#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include "CopyParam.h"
#include "HierarchicalStorage.h"
#include "TextsCore.h"

TCopyParamType::TCopyParamType()
{
  Default();
}

TCopyParamType::TCopyParamType(const TCopyParamType & Source)
{
  Assign(&Source);
}

TCopyParamType::~TCopyParamType()
{
}

void TCopyParamType::Default()
{
  // when changing defaults, make sure GetInfoStr() can handle it
  SetFileNameCase(ncNoChange);
  SetPreserveReadOnly(false);
  SetPreserveTime(true);
  FRights.SetNumber(TRights::rfDefault);
  SetPreserveRights(false); // Was True until #106
  SetIgnorePermErrors(false);
  FAsciiFileMask.SetMasks(UnicodeString(L"*.*html; *.htm; *.txt; *.php; *.php3; *.cgi; *.c; *.cpp; *.h; *.pas; "
    L"*.bas; *.tex; *.pl; *.js; .htaccess; *.xtml; *.css; *.cfg; *.ini; *.sh; *.xml"));
  SetTransferMode(tmBinary);
  SetAddXToDirectories(true);
  SetResumeSupport(rsSmart);
  SetResumeThreshold(100 * 1024); // (100 KB)
  SetInvalidCharsReplacement(TokenReplacement);
  SetLocalInvalidChars(LOCAL_INVALID_CHARS);
  SetCalculateSize(true);
  SetFileMask(L"*.*");
  GetIncludeFileMask().SetMasks(L"");
  SetTransferSkipList(nullptr);
  SetTransferResumeFile(L"");
  SetClearArchive(false);
  SetRemoveCtrlZ(false);
  SetRemoveBOM(false);
  SetCPSLimit(0);
  SetNewerOnly(false);
}

UnicodeString TCopyParamType::GetInfoStr(
  const UnicodeString & Separator, intptr_t Options) const
{
  UnicodeString Result;
  bool SomeAttrIncluded;
  DoGetInfoStr(Separator, Options, Result, SomeAttrIncluded);
  return Result;
}

bool TCopyParamType::AnyUsableCopyParam(intptr_t Options) const
{
  UnicodeString Result;
  bool SomeAttrIncluded;
  DoGetInfoStr(L";", Options, Result, SomeAttrIncluded);
  return SomeAttrIncluded;
}

void TCopyParamType::DoGetInfoStr(
  const UnicodeString & Separator, intptr_t Options,
  UnicodeString & Result, bool & SomeAttrIncluded) const
{
  TCopyParamType Defaults;

  bool SomeAttrExcluded = false;
  SomeAttrIncluded = false;
  #define ADD(STR, EXCEPT) \
    if (FLAGCLEAR(Options, EXCEPT)) \
    { \
      AddToList(Result, (STR), Separator); \
      SomeAttrIncluded = true; \
    } \
    else \
    { \
      SomeAttrExcluded = true; \
    }

  bool TransferModeDiffers =
    ((GetTransferMode() != Defaults.GetTransferMode()) ||
     ((GetTransferMode() == tmAutomatic) && !(GetAsciiFileMask() == Defaults.GetAsciiFileMask())));

  if (FLAGCLEAR(Options, cpaIncludeMaskOnly | cpaNoTransferMode))
  {
    // Adding Transfer type unconditionally
    bool FormatMask;
    int Ident;
    switch (GetTransferMode())
    {
      case tmBinary:
        FormatMask = false;
        Ident = 2;
        break;
      case tmAscii:
        FormatMask = false;
        Ident = 3;
        break;
      case tmAutomatic:
      default:
        FormatMask = !(GetAsciiFileMask() == Defaults.GetAsciiFileMask());
        Ident = FormatMask ? 4 : 5;
        break;
    }
    UnicodeString S = FORMAT(LoadStrPart(COPY_INFO_TRANSFER_TYPE2, 1).c_str(),
      LoadStrPart(COPY_INFO_TRANSFER_TYPE2, Ident).c_str());
    if (FormatMask)
    {
      S = FORMAT(S.c_str(), GetAsciiFileMask().GetMasks().c_str());
    }
    AddToList(Result, S, Separator);

    if (TransferModeDiffers)
    {
      ADD(L"", cpaIncludeMaskOnly | cpaNoTransferMode);
    }
  }
  else
  {
    if (TransferModeDiffers)
    {
      SomeAttrExcluded = true;
    }
  }

  if (GetFileNameCase() != Defaults.GetFileNameCase())
  {
    ADD(FORMAT(LoadStrPart(COPY_INFO_FILENAME, 1).c_str(),
      LoadStrPart(COPY_INFO_FILENAME, GetFileNameCase() + 2).c_str()),
      cpaIncludeMaskOnly);
  }

  if ((GetInvalidCharsReplacement() == NoReplacement) !=
        (Defaults.GetInvalidCharsReplacement() == NoReplacement))
  {
    assert(GetInvalidCharsReplacement() == NoReplacement);
    if (GetInvalidCharsReplacement() == NoReplacement)
    {
      ADD(LoadStr(COPY_INFO_DONT_REPLACE_INV_CHARS).c_str(), cpaIncludeMaskOnly);
    }
  }

  if ((GetPreserveRights() != Defaults.GetPreserveRights()) ||
      (GetPreserveRights() &&
       ((GetRights() != Defaults.GetRights()) || (GetAddXToDirectories() != Defaults.GetAddXToDirectories()))))
  {
    assert(GetPreserveRights());

    if (GetPreserveRights())
    {
      UnicodeString RightsStr = GetRights().GetText();
      if (GetAddXToDirectories())
      {
        RightsStr += L", " + LoadStr(COPY_INFO_ADD_X_TO_DIRS);
      }
      ADD(FORMAT(LoadStr(COPY_INFO_PERMISSIONS).c_str(), RightsStr.c_str()),
        cpaIncludeMaskOnly | cpaNoRights);
    }
  }

  if (GetPreserveTime() != Defaults.GetPreserveTime())
  {
    ADD(LoadStr(GetPreserveTime() ? COPY_INFO_TIMESTAMP : COPY_INFO_DONT_PRESERVE_TIME).c_str(),
      cpaIncludeMaskOnly | cpaNoPreserveTime);
  }

  if ((GetPreserveRights() || GetPreserveTime()) &&
      (GetIgnorePermErrors() != Defaults.GetIgnorePermErrors()))
  {
    assert(GetIgnorePermErrors());

    if (GetIgnorePermErrors())
    {
      ADD(LoadStr(COPY_INFO_IGNORE_PERM_ERRORS).c_str(),
        cpaIncludeMaskOnly | cpaNoIgnorePermErrors);
    }
  }

  if (GetPreserveReadOnly() != Defaults.GetPreserveReadOnly())
  {
    assert(GetPreserveReadOnly());
    if (GetPreserveReadOnly())
    {
      ADD(LoadStr(COPY_INFO_PRESERVE_READONLY).c_str(),
        cpaIncludeMaskOnly | cpaNoPreserveReadOnly);
    }
  }

  if (GetCalculateSize() != Defaults.GetCalculateSize())
  {
    assert(!GetCalculateSize());
    if (!GetCalculateSize())
    {
      ADD(LoadStr(COPY_INFO_DONT_CALCULATE_SIZE).c_str(), cpaIncludeMaskOnly);
    }
  }

  if (GetClearArchive() != Defaults.GetClearArchive())
  {
    assert(GetClearArchive());
    if (GetClearArchive())
    {
      ADD(LoadStr(COPY_INFO_CLEAR_ARCHIVE),
        cpaIncludeMaskOnly | cpaNoClearArchive);
    }
  }

  if ((GetTransferMode() == tmAscii) || (GetTransferMode() == tmAutomatic))
  {
    if (GetRemoveBOM() != Defaults.GetRemoveBOM())
    {
      if (ALWAYS_TRUE(GetRemoveBOM()))
      {
        ADD(LoadStr(COPY_INFO_REMOVE_BOM),
          cpaIncludeMaskOnly | cpaNoRemoveBOM | cpaNoTransferMode);
      }
    }

    if (GetRemoveCtrlZ() != Defaults.GetRemoveCtrlZ())
    {
      if (ALWAYS_TRUE(GetRemoveCtrlZ()))
      {
        ADD(LoadStr(COPY_INFO_REMOVE_CTRLZ),
          cpaIncludeMaskOnly | cpaNoRemoveCtrlZ | cpaNoTransferMode);
      }
    }
  }

  if (!(GetIncludeFileMask() == Defaults.GetIncludeFileMask()))
  {
    ADD(FORMAT(LoadStr(COPY_INFO_FILE_MASK).c_str(), GetIncludeFileMask().GetMasks().c_str()),
      cpaNoIncludeMask);
  }

  assert(FTransferSkipList.get() == nullptr);
  assert(FTransferResumeFile.IsEmpty());

  if (GetCPSLimit() > 0)
  {
    ADD(FMTLOAD(COPY_INFO_CPS_LIMIT2, static_cast<int>(GetCPSLimit() / 1024)).c_str(), cpaIncludeMaskOnly);
  }

  if (GetNewerOnly() != Defaults.GetNewerOnly())
  {
    if (ALWAYS_TRUE(GetNewerOnly()))
    {
      ADD(StripHotkey(LoadStr(COPY_PARAM_NEWER_ONLY)), cpaIncludeMaskOnly | cpaNoNewerOnly);
    }
  }

  if (SomeAttrExcluded)
  {
    Result += (Result.IsEmpty() ? UnicodeString() : Separator) +
      FORMAT(LoadStrPart(COPY_INFO_NOT_USABLE, 1).c_str(),
        LoadStrPart(COPY_INFO_NOT_USABLE, (SomeAttrIncluded ? 2 : 3)).c_str());
  }
  else if (Result.IsEmpty())
  {
    Result = LoadStr(COPY_INFO_DEFAULT);
  }
  #undef ADD
}

void TCopyParamType::Assign(const TCopyParamType * Source)
{
  assert(Source != nullptr);
  #define COPY(Prop) Set ## Prop(Source->Get ## Prop())
  COPY(FileNameCase);
  COPY(PreserveReadOnly);
  COPY(PreserveTime);
  COPY(Rights);
  COPY(AsciiFileMask);
  COPY(TransferMode);
  COPY(AddXToDirectories);
  COPY(PreserveRights);
  COPY(IgnorePermErrors);
  COPY(ResumeSupport);
  COPY(ResumeThreshold);
  COPY(InvalidCharsReplacement);
  COPY(LocalInvalidChars);
  COPY(CalculateSize);
  COPY(FileMask);
  COPY(IncludeFileMask);
  COPY(TransferSkipList);
  COPY(TransferResumeFile);
  COPY(ClearArchive);
  COPY(RemoveCtrlZ);
  COPY(RemoveBOM);
  COPY(CPSLimit);
  COPY(NewerOnly);
  #undef COPY
}

TCopyParamType & TCopyParamType::operator =(const TCopyParamType & rhp)
{
  Assign(&rhp);
  return *this;
}

void TCopyParamType::SetLocalInvalidChars(const UnicodeString & Value)
{
  if (Value != GetLocalInvalidChars())
  {
    FLocalInvalidChars = Value;
    FTokenizibleChars = FLocalInvalidChars; // + TokenPrefix;
  }
}

bool TCopyParamType::GetReplaceInvalidChars() const
{
  return (GetInvalidCharsReplacement() != NoReplacement);
}

void TCopyParamType::SetReplaceInvalidChars(bool Value)
{
  if (GetReplaceInvalidChars() != Value)
  {
    SetInvalidCharsReplacement(Value ? TokenReplacement : NoReplacement);
  }
}

UnicodeString TCopyParamType::ValidLocalFileName(const UnicodeString & AFileName) const
{
  return ::ValidLocalFileName(AFileName, GetInvalidCharsReplacement(), FTokenizibleChars, LOCAL_INVALID_CHARS);
}

UnicodeString TCopyParamType::RestoreChars(const UnicodeString & AFileName) const
{
  UnicodeString Result = AFileName;
  if (GetInvalidCharsReplacement() == TokenReplacement)
  {
    wchar_t * InvalidChar = const_cast<wchar_t *>(Result.c_str());
    while ((InvalidChar = wcschr(InvalidChar, TokenPrefix)) != nullptr)
    {
      intptr_t Index = InvalidChar - Result.c_str() + 1;
      if (Result.Length() >= Index + 2)
      {
        UnicodeString Hex = Result.SubString(Index + 1, 2);
        wchar_t Char = static_cast<wchar_t>(HexToByte(Hex));
        if ((Char != L'\0') &&
            ((FTokenizibleChars.Pos(Char) > 0) ||
             (((Char == L' ') || (Char == L'.')) && (Index == Result.Length() - 2))))
        {
          Result[Index] = Char;
          Result.Delete(Index + 1, 2);
          InvalidChar = const_cast<wchar_t *>(Result.c_str() + Index);
        }
        else if ((Hex == L"00") &&
                 ((Index == Result.Length() - 2) || (Result[Index + 3] == L'.')) &&
                 IsReservedName(Result.SubString(1, Index - 1) + Result.SubString(Index + 3, Result.Length() - Index - 3 + 1)))
        {
          Result.Delete(Index, 3);
          InvalidChar = const_cast<wchar_t *>(Result.c_str() + Index - 1);
        }
        else
        {
          InvalidChar++;
        }
      }
      else
      {
        InvalidChar++;
      }
    }
  }
  return Result;
}

UnicodeString TCopyParamType::ValidLocalPath(const UnicodeString & APath) const
{
  UnicodeString Result;
  UnicodeString Path2 = APath;
  while (!Path2.IsEmpty())
  {
    if (!Result.IsEmpty())
    {
      Result += L"\\";
    }
    Result += ValidLocalFileName(CutToChar(Path2, L'\\', false));
  }
  return Result;
}

UnicodeString TCopyParamType::ChangeFileName(const UnicodeString & AFileName,
  TOperationSide Side, bool FirstLevel) const
{
  UnicodeString Result = AFileName;
  if (FirstLevel)
  {
    Result = MaskFileName(Result, GetFileMask());
  }
  switch (GetFileNameCase())
  {
    case ncUpperCase:
      Result = Result.UpperCase();
      break;
    case ncLowerCase:
      Result = Result.LowerCase();
      break;
    case ncFirstUpperCase:
      Result = Result.SubString(1, 1).UpperCase() +
        Result.SubString(2, Result.Length() - 1).LowerCase();
      break;
    case ncLowerCaseShort:
      if ((Result.Length() <= 12) && (Result.Pos(L".") <= 9) &&
          (Result == Result.UpperCase()))
      {
        Result = Result.LowerCase();
      }
      break;
    case ncNoChange:
    default:
      /*nothing*/
      break;
  }
  if (Side == osRemote)
  {
    Result = ValidLocalFileName(Result);
  }
  else
  {
    Result = RestoreChars(Result);
  }
  return Result;
}

bool TCopyParamType::UseAsciiTransfer(const UnicodeString & AFileName,
  TOperationSide Side, const TFileMasks::TParams & Params) const
{
  switch (GetTransferMode())
  {
    case tmBinary:
      return false;
    case tmAscii:
      return true;
    case tmAutomatic:
      return GetAsciiFileMask().Matches(AFileName, (Side == osLocal),
        false, &Params);
    default:
      FAIL;
      return false;
  }
}

TRights TCopyParamType::RemoteFileRights(uintptr_t Attrs) const
{
  TRights R = GetRights();
  if ((Attrs & faDirectory) && GetAddXToDirectories())
    R.AddExecute();
  return R;
}

UnicodeString TCopyParamType::GetLogStr() const
{
  wchar_t CaseC[] = L"NULFS";
  wchar_t ModeC[] = L"BAM";
  wchar_t ResumeC[] = L"YSN";
  // OpenArray (ARRAYOFCONST) supports only up to 19 arguments, so we had to split it
  return
   FORMAT(
      L"  PrTime: %s; PrRO: %s; Rght: %s; PrR: %s (%s); FnCs: %c; RIC: %c; "
      L"Resume: %c (%d); CalcS: %s; Mask: %s\n",
      BooleanToEngStr(GetPreserveTime()).c_str(),
      BooleanToEngStr(GetPreserveReadOnly()).c_str(),
      GetRights().GetText().c_str(),
      BooleanToEngStr(GetPreserveRights()).c_str(),
      BooleanToEngStr(GetIgnorePermErrors()).c_str(),
      CaseC[GetFileNameCase()],
      CharToHex(GetInvalidCharsReplacement()).c_str(),
      ResumeC[GetResumeSupport()],
      (int)GetResumeThreshold(),
      BooleanToEngStr(GetCalculateSize()).c_str(),
      GetFileMask().c_str()) +
    FORMAT(
      L"  TM: %c; ClAr: %s; RemEOF: %s; RemBOM: %s; CPS: %u; NewerOnly: %s; InclM: %s; ResumeL: %d\n"
      L"  AscM: %s\n",
      ModeC[GetTransferMode()],
      BooleanToEngStr(GetClearArchive()).c_str(),
      BooleanToEngStr(GetRemoveCtrlZ()).c_str(),
      BooleanToEngStr(GetRemoveBOM()).c_str(),
      int(GetCPSLimit()),
      BooleanToEngStr(GetNewerOnly()).c_str(),
      GetIncludeFileMask().GetMasks().c_str(),
      ((FTransferSkipList.get() != nullptr) ? FTransferSkipList->GetCount() : 0) + (!FTransferResumeFile.IsEmpty() ? 1 : 0),
      GetAsciiFileMask().GetMasks().c_str());
//  return FORMAT(
//    "  PrTime: %s; PrRO: %s; Rght: %s; PrR: %s (%s); FnCs: %c; RIC: %s; "
//       "Resume: %c (%d); CalcS: %s; Mask: %s\n"
//    "  TM: %c; ClAr: %s; RemEOF: %s; RemBOM: %s; CPS: %u; NewerOnly: %s; InclM: %s\n"
//    "  AscM: %s\n",
//    BooleanToEngStr(GetPreserveTime()).c_str(),
//    BooleanToEngStr(GetPreserveReadOnly()).c_str(),
//    GetRights().GetText().c_str(),
//    BooleanToEngStr(GetPreserveRights()).c_str(),
//    BooleanToEngStr(GetIgnorePermErrors()).c_str(),
//    CaseC[GetFileNameCase()],
//    CharToHex(GetInvalidCharsReplacement()).c_str(),
//    ResumeC[GetResumeSupport()],
//    (int)GetResumeThreshold(),
//    BooleanToEngStr(GetCalculateSize()).c_str(),
//    GetFileMask().c_str(),
//    ModeC[GetTransferMode()],
//    BooleanToEngStr(GetClearArchive()).c_str(),
//    BooleanToEngStr(GetRemoveCtrlZ()).c_str(),
//    BooleanToEngStr(GetRemoveBOM()).c_str(),
//    int(GetCPSLimit()),
//    BooleanToEngStr(GetNewerOnly()).c_str(),
//    GetIncludeFileMask().GetMasks().c_str(),
//    GetAsciiFileMask().GetMasks().c_str());
}

DWORD TCopyParamType::LocalFileAttrs(const TRights & Rights) const
{
  DWORD Result = 0;
  if (GetPreserveReadOnly() && !Rights.GetRight(TRights::rrUserWrite))
  {
    Result |= faReadOnly;
  }
  return Result;
}

bool TCopyParamType::AllowResume(int64_t Size) const
{
  switch (GetResumeSupport())
  {
    case rsOn:
      return true;
    case rsOff:
      return false;
    case rsSmart:
      return (Size >= GetResumeThreshold());
    default:
      FAIL;
      return false;
  }
}

bool TCopyParamType::AllowAnyTransfer() const
{
  return
    GetIncludeFileMask().GetMasks().IsEmpty() &&
    ((FTransferSkipList.get() == nullptr) || (FTransferSkipList->GetCount() == 0)) &&
    FTransferResumeFile.IsEmpty();
}

bool TCopyParamType::AllowTransfer(const UnicodeString & AFileName,
  TOperationSide Side, bool Directory, const TFileMasks::TParams & Params) const
{
  bool Result = true;
  if (!GetIncludeFileMask().GetMasks().IsEmpty())
  {
    Result = GetIncludeFileMask().Matches(AFileName, (Side == osLocal),
      Directory, &Params);
  }
  return Result;
}

bool TCopyParamType::SkipTransfer(
  const UnicodeString & AFileName, bool Directory) const
{
  bool Result = false;
  // we deliberately do not filter directories, as path is added to resume list
  // when a transfer of file or directory is started,
  // so for directories we need to recurse and check every single file
  if (!Directory && FTransferSkipList.get() != nullptr)
  {
    Result = (FTransferSkipList->IndexOf(AFileName) >= 0);
  }
  return Result;
}

bool TCopyParamType::ResumeTransfer(const UnicodeString & AFileName) const
{
  // Returning true has the same effect as cpResume
  return
    (AFileName == FTransferResumeFile) &&
    ALWAYS_TRUE(!FTransferResumeFile.IsEmpty());
}

TStrings * TCopyParamType::GetTransferSkipList() const
{
  return FTransferSkipList.get();
}

void TCopyParamType::SetTransferSkipList(TStrings * Value)
{
  if ((Value == nullptr) || (Value->GetCount() == 0))
  {
    FTransferSkipList.reset(nullptr);
  }
  else
  {
    FTransferSkipList.reset(new TStringList());
    FTransferSkipList->AddStrings(Value);
    FTransferSkipList->SetSorted(true);
  }
}

void TCopyParamType::Load(THierarchicalStorage * Storage)
{
  SetAddXToDirectories(Storage->ReadBool("AddXToDirectories", GetAddXToDirectories()));
  GetAsciiFileMask().SetMasks(Storage->ReadString("Masks", GetAsciiFileMask().GetMasks()));
  SetFileNameCase(static_cast<TFileNameCase>(Storage->ReadInteger("FileNameCase", GetFileNameCase())));
  SetPreserveReadOnly(Storage->ReadBool("PreserveReadOnly", GetPreserveReadOnly()));
  SetPreserveTime(Storage->ReadBool("PreserveTime", GetPreserveTime()));
  SetPreserveRights(Storage->ReadBool("PreserveRights", GetPreserveRights()));
  SetIgnorePermErrors(Storage->ReadBool("IgnorePermErrors", GetIgnorePermErrors()));
  FRights.SetText(Storage->ReadString("Text", GetRights().GetText()));
  SetTransferMode(static_cast<TTransferMode>(Storage->ReadInteger("TransferMode", GetTransferMode())));
  SetResumeSupport(static_cast<TResumeSupport>(Storage->ReadInteger("ResumeSupport", GetResumeSupport())));
  SetResumeThreshold(Storage->ReadInt64("ResumeThreshold", GetResumeThreshold()));
  SetInvalidCharsReplacement(static_cast<wchar_t>(Storage->ReadInteger("ReplaceInvalidChars",
    static_cast<int>(GetInvalidCharsReplacement()))));
  SetLocalInvalidChars(Storage->ReadString("LocalInvalidChars", GetLocalInvalidChars()));
  SetCalculateSize(Storage->ReadBool("CalculateSize", GetCalculateSize()));
  if (Storage->ValueExists("IncludeFileMask"))
  {
    GetIncludeFileMask().SetMasks(Storage->ReadString("IncludeFileMask", GetIncludeFileMask().GetMasks()));
  }
  else if (Storage->ValueExists("ExcludeFileMask"))
  {
    UnicodeString ExcludeFileMask = Storage->ReadString("ExcludeFileMask", UnicodeString());
    if (!ExcludeFileMask.IsEmpty())
    {
      bool NegativeExclude = Storage->ReadBool("NegativeExclude", false);
      if (NegativeExclude)
      {
        GetIncludeFileMask().SetMasks(ExcludeFileMask);
      }
      // convert at least simple cases to new format
      else if (ExcludeFileMask.Pos(INCLUDE_EXCLUDE_FILE_MASKS_DELIMITER) == 0)
      {
        GetIncludeFileMask().SetMasks(UnicodeString(INCLUDE_EXCLUDE_FILE_MASKS_DELIMITER) + ExcludeFileMask);
      }
    }
  }
  SetTransferSkipList(nullptr);
  SetTransferResumeFile(L"");
  SetClearArchive(Storage->ReadBool("ClearArchive", GetClearArchive()));
  SetRemoveCtrlZ(Storage->ReadBool("RemoveCtrlZ", GetRemoveCtrlZ()));
  SetRemoveBOM(Storage->ReadBool("RemoveBOM", GetRemoveBOM()));
  SetCPSLimit(Storage->ReadInteger("CPSLimit", GetCPSLimit()));
  SetNewerOnly(Storage->ReadBool("NewerOnly", GetNewerOnly()));
}

void TCopyParamType::Save(THierarchicalStorage * Storage) const
{
  Storage->WriteBool(L"AddXToDirectories", GetAddXToDirectories());
  Storage->WriteString(L"Masks", GetAsciiFileMask().GetMasks());
  Storage->WriteInteger(L"FileNameCase", GetFileNameCase());
  Storage->WriteBool(L"PreserveReadOnly", GetPreserveReadOnly());
  Storage->WriteBool(L"PreserveTime", GetPreserveTime());
  Storage->WriteBool(L"PreserveRights", GetPreserveRights());
  Storage->WriteBool(L"IgnorePermErrors", GetIgnorePermErrors());
  Storage->WriteString(L"Text", GetRights().GetText());
  Storage->WriteInteger(L"TransferMode", GetTransferMode());
  Storage->WriteInteger(L"ResumeSupport", GetResumeSupport());

  Storage->WriteInt64(L"ResumeThreshold", GetResumeThreshold());
  Storage->WriteInteger(L"ReplaceInvalidChars", static_cast<uint32_t>(GetInvalidCharsReplacement()));
  Storage->WriteString(L"LocalInvalidChars", GetLocalInvalidChars());
  Storage->WriteBool(L"CalculateSize", GetCalculateSize());
  Storage->WriteString(L"IncludeFileMask", GetIncludeFileMask().GetMasks());
  Storage->DeleteValue(L"ExcludeFileMask"); // obsolete
  Storage->DeleteValue(L"NegativeExclude"); // obsolete
  assert(FTransferSkipList.get() == nullptr);
  assert(FTransferResumeFile.IsEmpty());
  Storage->WriteBool(L"ClearArchive", GetClearArchive());
  Storage->WriteBool(L"RemoveCtrlZ", GetRemoveCtrlZ());
  Storage->WriteBool(L"RemoveBOM", GetRemoveBOM());
  Storage->WriteInteger(L"CPSLimit", GetCPSLimit());
  Storage->WriteBool(L"NewerOnly", GetNewerOnly());
}

#define C(Property) (Get ## Property() == rhp.Get ## Property())
bool TCopyParamType::operator==(const TCopyParamType & rhp) const
{
  assert(FTransferSkipList.get() == nullptr);
  assert(FTransferResumeFile.IsEmpty());
  assert(rhp.FTransferSkipList.get() == nullptr);
  assert(rhp.FTransferResumeFile.IsEmpty());
  return
    C(AddXToDirectories) &&
    C(AsciiFileMask) &&
    C(FileNameCase) &&
    C(PreserveReadOnly) &&
    C(PreserveTime) &&
    C(PreserveRights) &&
    C(IgnorePermErrors) &&
    C(Rights) &&
    C(TransferMode) &&
    C(ResumeSupport) &&
    C(ResumeThreshold) &&
    C(InvalidCharsReplacement) &&
    C(LocalInvalidChars) &&
    C(CalculateSize) &&
    C(IncludeFileMask) &&
    C(ClearArchive) &&
    C(RemoveCtrlZ) &&
    C(RemoveBOM) &&
    C(CPSLimit) &&
    C(NewerOnly) &&
    true;
}
#undef C

uintptr_t GetSpeedLimit(const UnicodeString & Text)
{
  uintptr_t Result = 0;
  if (AnsiSameText(Text, LoadStr(SPEED_UNLIMITED)))
  {
    Result = 0;
  }
  else
  {
    int64_t SSpeed = 0;
    if (!TryStrToInt(Text, SSpeed) ||
         (SSpeed < 0))
    {
      throw Exception(FMTLOAD(SPEED_INVALID, Text.c_str()));
    }
    Result = static_cast<uintptr_t>(SSpeed);
  }
  return Result * 1024;
}

UnicodeString SetSpeedLimit(uintptr_t Limit)
{
  UnicodeString Result;
  if (Limit == 0)
  {
    Result = LoadStr(SPEED_UNLIMITED);
  }
  else
  {
    Result = ::IntToStr(Limit / 1024);
  }
  return Result;
}

NB_IMPLEMENT_CLASS(TCopyParamType, NB_GET_CLASS_INFO(TObject), nullptr)

