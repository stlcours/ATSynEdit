unit atsynedit_form_complete_synwrite;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, StrUtils, Graphics,
  Dialogs,
  ATSynEdit;

procedure DoEditorCompletionFromAcp(AEdit: TATSynEdit;
  const AFilenameAcp: string; ACaseSens, AIsPascal: boolean);


implementation

uses
  ATStringProc,
  ATSynEdit_form_complete;

type
  { TAcp }

  TAcp = class
  private
    ListAcpType: TStringlist;
    ListAcpText: TStringlist;
    ListAcpDesc: TStringlist;
    FWordChars: string;
    procedure DoLoadAcpFile(const fn: string; IsPascal: boolean);
    procedure DoOnGetCompleteProp(Sender: TObject; out AText: string; out
      ACharsLeft, ACharsRight: integer);
  public
    Ed: TATSynEdit;
    CaseSens: boolean;
    constructor Create; virtual;
    destructor Destroy; override;
  end;

var
  Acp: TAcp = nil;

//parse control string from .acp file (starts with #)
procedure SParseString_AcpControlLine(const s: string;
  var WordChars: string;
  var IsBracketSep: boolean);
var
  n: Integer;
begin
  if SBegin(s, '#chars') then
  begin
    WordChars:= '';
    IsBracketSep:= true;
    n:= Pos(' ', s);
    if n>0 then
    begin
      WordChars:= Copy(s, n+1, MaxInt);
      IsBracketSep:= Pos('(', WordChars)=0;
    end;
  end;
end;


//parse string from .acp file
procedure SParseString_AcpStd(
  const S: string;
  IsBracketSep: boolean;
  out SType, SId, SPar, SHint: string);
const
  cMaxHintLen = 300;
var
  a, b, c: Integer;
begin
  SType:= '';
  SId:= '';
  SPar:= '';
  SHint:= '';
  if Trim(s)='' then Exit;

  a:= PosEx(' ', s, 1);
  b:= PosEx(' ', s, a+1);
  if b=0 then
    b:= Length(s)+1;

  if IsBracketSep then
  begin
    c:= PosEx('(', s, a+1);
    if (c<b) and (c<>0) then
      b:= c;
  end;

  c:= PosEx('|', s, b);
  if c=0 then
    c:= MaxInt div 2;

  SType:= Copy(s, 1, a-1);
  SId:= Copy(s, a+1, b-a-1);
  SPar:= Copy(s, b, c-b);
  SHint:= Copy(s, c+1, cMaxHintLen);

  SReplaceAllPercentChars(SId);
  SReplaceAllPercentChars(SPar);

  SReplaceAll(SPar, ';', ','); //Pascal lexer has ";" param separator
  SReplaceAll(SPar, '[,', ',['); //for optional params
end;


procedure TAcp.DoLoadAcpFile(const fn: string; IsPascal: boolean);
var
  List: TStringList;
  s, SType, SText, SPar, SHint: string;
  IsBracketSep: boolean;
  i: Integer;
begin
  ListAcpType.Clear;
  ListAcpText.Clear;
  ListAcpDesc.Clear;

  FWordChars:= '';
  IsBracketSep:= true;

  List:= TStringList.Create;
  try
    List.LoadFromFile(fn);
    for i:= 0 to List.Count-1 do
    begin
      s:= List[i];
      if s='' then
        Continue;

      if s[1]='#' then
      begin
        SParseString_AcpControlLine(s, FWordChars, IsBracketSep);
        Continue;
      end;

      SParseString_AcpStd(s, IsBracketSep, SType, SText, SPar, SHint);
      if SText<>'' then
      begin
        if IsPascal then
        begin
          SDeleteFrom(SText, ':');
          if Pos('):', SPar)>0 then
          begin
            SDeleteFrom(SPar, '):');
            SPar:= SPar+')';
          end;
        end;

        ListAcpType.Add(SType);
        ListAcpText.Add(SText);
        ListAcpDesc.Add(SPar+'|'+SHint);
      end;
    end;
  finally
    FreeAndNil(List);
  end;
end;


procedure TAcp.DoOnGetCompleteProp(Sender: TObject;
  out AText: string; out ACharsLeft, ACharsRight: integer);
var
  s_line, s_word: atString;
  n: integer;
  s_type, s_text, s_desc: string;
  ok: boolean;
begin
  AText:= '';
  ACharsLeft:= 0;
  ACharsRight:= 0;

  s_line:= Ed.Strings.Lines[Ed.Carets[0].PosY];
  s_word:= '';

  n:= Ed.Carets[0].PosX;
  if (n>Length(s_line)) then exit;

  while (n>0) and (IsCharWord(s_line[n], FWordChars)) do
  begin
    s_word:= s_line[n]+s_word;
    Dec(n);
    Inc(ACharsLeft);
  end;

  n:= Ed.Carets[0].PosX;
  while (n<Length(s_line)) and (IsCharWord(s_line[n+1], FWordChars)) do
  begin
    Inc(n);
    Inc(ACharsRight);
  end;

  for n:= 0 to ListAcpText.Count-1 do
  begin
    s_type:= ListAcpType[n];
    s_text:= ListAcpText[n];
    s_desc:= ListAcpDesc[n];

    if s_word<>'' then
    begin
      if CaseSens then
        ok:= SBegin(s_text, s_word)
      else
        ok:= SBegin(UpperCase(s_text), UpperCase(s_word));
      if not ok then Continue;
    end;

    AText:= AText+s_type+'|'+s_text+'|'+s_desc+#13;
  end;
end;

constructor TAcp.Create;
begin
  inherited;
  ListAcpType:= TStringlist.create;
  ListAcpText:= TStringlist.create;
  ListAcpDesc:= TStringlist.create;
end;

destructor TAcp.Destroy;
begin
  FreeAndNil(ListAcpType);
  FreeAndNil(ListAcpText);
  FreeAndNil(ListAcpDesc);
  inherited;
end;

procedure DoEditorCompletionFromAcp(AEdit: TATSynEdit;
  const AFilenameAcp: string; ACaseSens, AIsPascal: boolean);
begin
  if not FileExists(AFilenameAcp) then exit;
  Acp.DoLoadAcpFile(AFilenameAcp, AIsPascal);
  Acp.Ed:= AEdit;
  Acp.CaseSens:= ACaseSens;
  DoEditorCompletionListbox(AEdit, AEdit, @Acp.DoOnGetCompleteProp);
end;

initialization
  Acp:= TAcp.Create;

  cCompleteFormSizeX:= 550;
  cCompleteFontStyles[0]:= [];
  cCompleteColorFont[0]:= clPurple;
  cCompleteColorFont[1]:= clBlack;
  cCompleteColorFont[2]:= clGray;
  cCompleteColorFont[3]:= clGreen;

finalization
  FreeAndNil(Acp);

end.

