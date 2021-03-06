var
  cBitmapWait: TPortableNetworkGraphic = nil;
  cBitmapSaving: TPortableNetworkGraphic = nil;
  cBitmapNiceScroll: TPortableNetworkGraphic = nil;
  cBitmapFoldPlus: TPortableNetworkGraphic = nil;
  cBitmapFoldMinus: TPortableNetworkGraphic = nil;

const
  crNiceScrollNone  = TCursor(-40);
  crNiceScrollUp    = TCursor(-41);
  crNiceScrollDown  = TCursor(-42);
  crNiceScrollLeft  = TCursor(-43);
  crNiceScrollRight = TCursor(-44);

const
  //under Mac don't use Ctrl key, use Meta key as default (e.g. Meta+C, Meta+A)
  ssXControl = {$ifndef darwin} ssCtrl {$else} ssMeta {$endif};

{ TATFoldedMarks }

function TATFoldedMarks.FindByCoord(ACoord: TPoint): TATFoldedMark;
var
  Mark: TATFoldedMark;
  i: integer;
begin
  for i:= 0 to Count-1 do
  begin
    Mark:= Items[i];
    if PtInRect(Mark.Coord, ACoord) then
      exit(Mark);
  end;
  Result.InitNone;
end;

{ TATFoldedMark }

procedure TATFoldedMark.Init(const ACoord: TRect; ALineFrom, ALineTo: integer);
begin
  Coord:= ACoord;
  LineFrom:= ALineFrom;
  LineTo:= ALineTo;
end;

procedure TATFoldedMark.InitNone;
begin
  Init(Rect(0, 0, 0, 0), -1, -1);
end;

function TATFoldedMark.IsInited: boolean;
begin
  Result:= LineFrom>=0;
end;

class operator TATFoldedMark.=(const a, b: TATFoldedMark): boolean;
begin
  Result:= false;
end;

{ TATSynScrollInfo }

procedure TATSynScrollInfo.Clear;
begin
  NPos:= 0;
  NPosLast:= 1;
  NMax:= 1;
  NPage:= 1;
  NPixelOffset:= 0;
  SmoothCharSize:= 1;
  SmoothPos:= 0;
  SmoothPosLast:= 1;
  SmoothMax:= 1;
  SmoothPage:= 1;
end;

function TATSynScrollInfo.TopGapVisible: boolean;
begin
  Result:= NPos=-1;
end;

function TATSynScrollInfo.TotalOffset: integer;
begin
  Result:= NPos*SmoothCharSize + NPixelOffset;
end;

class operator TATSynScrollInfo.= (const A, B: TATSynScrollInfo): boolean;
begin
  Result:=
    (A.Vertical=B.Vertical) and
    (A.NPos=B.NPos) and
    (A.NMax=B.NMax) and
    (A.NPage=B.NPage) and
    (A.NPixelOffset=B.NPixelOffset);
end;

procedure InitClipboardFormat; inline;
begin
  cATClipboardFormatId:= RegisterClipboardFormat('Application/X-Laz-ATSynEdit-Block');
end;

function GetResourceScaleSuffix: string;
var
  N: integer;
begin
  N:= Screen.PixelsPerInch;
  if N>=96*2 then
    Result:= '_200'
  else
  if N>=(96 * 3 div 2) then
    Result:= '_150'
  else
    Result:= '';
end;

procedure InitResources;
var
  Suffix: string;
begin
  Suffix:= GetResourceScaleSuffix;

  Screen.Cursors[crNiceScrollNone]:= LoadCursor(HInstance, 'ATSYN_MOVE');
  Screen.Cursors[crNiceScrollUp]:= LoadCursor(HInstance, 'ATSYN_MOVE_U');
  Screen.Cursors[crNiceScrollDown]:= LoadCursor(HInstance, 'ATSYN_MOVE_D');
  Screen.Cursors[crNiceScrollLeft]:= LoadCursor(HInstance, 'ATSYN_MOVE_L');
  Screen.Cursors[crNiceScrollRight]:= LoadCursor(HInstance, 'ATSYN_MOVE_R');

  cBitmapNiceScroll:= TPortableNetworkGraphic.Create;
  cBitmapNiceScroll.LoadFromResourceName(HInstance, 'ATSYN_SCROLLMARK'+Suffix);

  cBitmapWait:= TPortableNetworkGraphic.Create;
  cBitmapWait.LoadFromResourceName(HInstance, 'ATSYN_WAIT');

  cBitmapSaving:= TPortableNetworkGraphic.Create;
  cBitmapSaving.LoadFromResourceName(HInstance, 'ATSYN_SAVE');

  cBitmapFoldPlus:= TPortableNetworkGraphic.Create;
  cBitmapFoldPlus.LoadFromResourceName(HInstance, 'ATSYN_FOLD_P'+Suffix);

  cBitmapFoldMinus:= TPortableNetworkGraphic.Create;
  cBitmapFoldMinus.LoadFromResourceName(HInstance, 'ATSYN_FOLD_M'+Suffix);
end;

procedure FreeResources;
begin
  FreeAndNil(cBitmapWait);
  FreeAndNil(cBitmapSaving);
  FreeAndNil(cBitmapNiceScroll);
  FreeAndNil(cBitmapFoldPlus);
  FreeAndNil(cBitmapFoldMinus);
end;

function IsDoubleBufferedNeeded: boolean;
begin
  Result:= WidgetSet.GetLCLCapability(lcCanDrawOutsideOnPaint) = LCL_CAPABILITY_YES;
end;


procedure InitMouseActions(var M: TATMouseActions);
  //
  procedure Add(AId: TATMouseActionId; AState: TShiftState);
  begin
    SetLength(M, Length(M)+1);
    M[High(M)].MouseActionId:= AId;
    M[High(M)].MouseState:= AState;
  end;
  //
begin
  SetLength(M, 0);

  Add(cMouseActionClickSimple, [ssLeft]);
  Add(cMouseActionClickRight, [ssRight]);
  Add(cMouseActionClickAndSelNormalBlock, [ssLeft, ssShift]);
  Add(cMouseActionClickAndSelVerticalBlock, [ssLeft, ssAlt, ssShift]);
  Add(cMouseActionMakeCaret, [ssLeft, ssXControl]);
  Add(cMouseActionMakeCaretsColumn, [ssLeft, ssXControl, ssShift]);
  Add(cMouseActionNiceScrolling, [ssMiddle]);

  Add(cMouseActionClickSimple, [ssLeft, ssAlt]);
  Add(cMouseActionClickSimple, [ssLeft, ssAltGr]);
  Add(cMouseActionClickSimple, [ssLeft, ssAlt, ssXControl]);
  Add(cMouseActionClickSimple, [ssLeft, ssAlt, ssShift, ssXControl]);
end;


function GetMouseActionId(const M: TATMouseActions; State: TShiftState): TATMouseActionId;
var
  i: integer;
begin
  Result:= cMouseActionNone;
  if State=[] then exit;

  for i:= Low(M) to High(M) do
    if M[i].MouseState=State then
      exit(M[i].MouseActionId);
end;

function IsPressedCtrl: boolean;
begin
  {$ifdef darwin}
  //on macOS use Command-key instead of Ctrl
  Result:=
    (GetKeyState(VK_LWIN)<0) or
    (GetKeyState(VK_RWIN)<0);
  {$else}
  Result:= GetKeyState(VK_CONTROL)<0;
  {$endif}
end;


