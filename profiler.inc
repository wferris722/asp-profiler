<%

Const codeLiteral = 0
Const codeVBScript = 1
Const codeJScript = 2

Class CodeSection
  Public StartIndex
  Public StartString
  Public Content
  Public EndIndex
  Public EndString
  Private inLineCount
  Public SectionType

  Private Sub Class_Initialize()
    inLineCount = Null
  End Sub

  Public Property get LineCount()
    If IsNull(inLineCount) Then
      inLineCount = 0
      Dim i: i = 1
      Dim t
      Do While True
        t = InStr(i, Content, vbCrLf, vbBinaryCompare)
        If t = 0 Then
          Exit Do
        End If
        inLineCount = inLineCount + 1
        i = t + 2
      Loop
    End If
    LineCount = inLineCount
  End Property

  Public Property get StartLength()
    StartLength = len(StartString)
  End Property

  Public Property get EndLength()
    EndLength = len(EndString)
  End Property

  Public Sub SetLiteral(indexStart, indexEnd, sCode)
    StartIndex = indexStart
    EndIndex = indexEnd
    StartString = ""
    EndString = ""
    SectionType = codeLiteral
    Content = Mid(sCode, startIndex, EndIndex - startIndex)
  End Sub

  Public Sub SetCode(StartMatch, EndMatch, sCode)
    StartIndex = StartMatch.FirstIndex + 1
    EndIndex = EndMatch.FirstIndex + 1
    StartString = StartMatch.Value
    EndString = EndMatch.Value
    Content = Mid(sCode, StartIndex + StartLength, EndIndex - (StartIndex + StartLength))
    DetermineType
  End Sub

  Public Sub DetermineType()
    dim isScript: isScript = true
    If StartString = "<" & "%" Then
      SectionType = codeVbScript
      isScript = false
    ElseIf Not StringContains(StartString, "runat") Then
      SectionType = codeLiteral
      isScript = false
    ElseIf StringContains(StartString, "vbscript") Then
      SectionType = codeVbScript
    ElseIf StringContains(StartString, "jscript") Then
      SectionType = codeJScript
    Else
      Err.Raise 1, "CodeSection.DetermineType", "Unable to determine the code type of the start string" & vbCrLf & DumpFields()
    End If

    If EndString = "%" & ">" And IsScript Then
      Err.Raise 2, "CodeSection.DetermineType", "Opened with script tag, but closing tag was %" & ">" & vbCrLf & DumpFields()
    ElseIf StringContains(EndString, "script") And Not IsScript Then
      Err.Raise 3, "CodeSection.DetermineType", "Opened with <" & "%, but closing tag was a script tag" & vbCrLf & DumpFields()
    End If
  End Sub

  Private Function StringContains(subject, target) 
    StringContains = (InStr(1, subject, target, vbTextCompare) <> 0)
  End Function

  Private Function DumpFields()
    DumpFields = "" & _
      "StartIndex: " & StartIndex & vbCrLf & _
      "EndIndex: " & EndIndex & vbCrLF & _
      "StartString: '" & Server.HTMLEncode(StartString) & "'" & vbCrLf & _
      "EndString: '" & Server.HTMLEncode(EndString) & "'" & vbCrLf
  End Function

End Class


Const sInternalCount = "#LINECOUNT#"
Const sInternalFileName = "#FILENAME#"
Class ProfilerBase
    Private FileSystemObject
    Private ScriptStartRegex
    Private ScriptEndRegex

    Private Sub Class_Initialize()
        Set FileSystemObject = Server.CreateObject("Scripting.FileSystemObject")
        Set ScriptStartRegex = new RegExp
        Set ScriptEndRegex = new RegExp
        ScriptStartRegex.Global = True
        ScriptStartRegex.IgnoreCase = True
        ScriptStartRegex.MultiLine = True
        'ScriptStartRegex.Pattern = "(^\s*<\s*script\s+(runat=(""|)server(""|)\s+language=(""|)vbscript(""|)|language=(""|)vbscript(""|)\s+runat=(""|)server(""|))\s*>|<" & "%)"
        ScriptStartRegex.Pattern = "(<\s*script[^>]*>|<" & "%)"
        ScriptEndRegex.Global = True
        ScriptEndRegex.IgnoreCase = True
        ScriptEndRegex.MultiLine = True
        ScriptEndRegex.Pattern = "(%" & ">|<\s*\/script\s*>)"
    End Sub


    Public Function ToWebPath(path)
        ToWebPath = Replace(path, "\", "/")
        If InStr(1, ToWebPath, "?") > 0 Then ToWebPath = Left(ToWebPath, InStr(1, ToWebPath, "?") - 1)
        If Left(ToWebPath, 1) <> "/" Then ToWebPath = "/" & ToWebPath
    End Function

    Public Function PathExists(path) 
        PathExists = FileSystemObject.FileExists(Server.MapPath(path))
    End Function

    Public Function JSONString(value)
        If IsNull(value) or isEmpty(value) then
            JSONString = "null"
            Exit Function
        End If
        JSONString = """"
        Dim strValue: strValue = CSTR(value)
        Dim length: length = len(strValue)
        Dim i, char
        for i = 1 to length
            char = mid(strValue, i, 1)
            select case char
                case """"
                    char = "\"""
                case "\"
                    char = "\\"
                case "/" 'Not standard, but needed for escaping </script> blocks inside strings
                    char = "\/"
                case chr(10)
                    char = "\n"
                case chr(13)
                    char = "\r"
                case chr(8)
                    char = "\n"
                case chr(12)
                    char = "\f"
                case chr(9)
                    char = "\t"
            end select
            JSONString = JSONString & char
        next
        JSONString = JSONString & """"
    End Function


Public Sub InstrumentCode(path,ByRef code, ByRef instrumentedCode, ByRef dictLimits)
    code = LoadFile(Server.MapPath(path))
    Set dictLimits = Server.CreateObject("Scripting.Dictionary")
    ResolveAllIncludes code, path, dictLimits
    
    instrumentedCode = ProfileCode(code, FileSystemObject.GetBaseName(Server.MapPath(path)))
End Sub

Sub FindInclude(sFile, iPos, iLength, sIncPath)
   Dim r, ms, m, i, j, s
   Set r = New RegExp
   r.Pattern = "<!--\s*#include\s+(file|virtual)\s*=\s*"".+""\s*-->"
   r.IgnoreCase = True
   r.Global = False
   Set ms = r.Execute(sFile)
   If ms.Count = 0 Then 
      iPos = 0
   Else
      For Each m In ms
         iPos = m.FirstIndex + 1
         iLength = m.Length
         s = m.Value
         i = Instr(1, s, """")
         j = Instr(i+1, s, """")
         sIncPath = Mid(s, i+1, j-i-1)
         i = Instr(1, s, "virtual", vbTextCompare)
         If i > 0 And Left(sIncPath,1) <> "/" Then sIncPath = "/" & sIncPath
      Next
   End If
End Sub

Function ResolveInclude(sIncPath, dictLimits)
   Dim sIncText, f
   On Error Resume Next
   Set f = FileSystemObject.OpenTextFile(Server.MapPath(sIncPath))
   If Err Then Response.Write "Cannot open " & sIncPath : Response.End
   sIncText = ConvertCrLf(f.ReadAll)
   ResolveAllIncludes sIncText, sIncPath, dictLimits
   ResolveInclude = sIncText
End Function

Sub ResolveAllIncludes(sFile, sWebPath, dictLimits)
   Dim iPos, iLength, sIncText, sIncPath
   Dim sBefore, iIncFirstLine, iTotalIncLines, iIncLines, dictIncLimits, arrDictItem
   
   iTotalIncLines = 0
   dictLimits.Add dictLimits.Count, Array(1, sWebPath, 1)
   
   FindInclude sFile, iPos, iLength, sIncPath
   Do Until iPos = 0
      If Left(sIncPath,1) <> "/" Then sIncPath = FileSystemObject.GetParentFolderName(sWebPath) & "/" & sIncPath
      sBefore = Left(sFile, iPos-1)
      
      iIncFirstLine = CountLines(sBefore)
      Set dictIncLimits = Server.CreateObject("Scripting.Dictionary")
      
      sIncText = ResolveInclude(sIncPath, dictIncLimits)
      sFile = sBefore & sIncText & Mid(sFile, iPos + iLength)
      
      For Each arrDictItem In dictIncLimits.Items
         arrDictItem(0) = arrDictItem(0) + iIncFirstLine - 1
         dictLimits.Add dictLimits.Count, arrDictItem
      Next
      iIncLines = CountLines(sIncText)
      dictLimits.Add dictLimits.Count, Array(iIncFirstLine + iIncLines, sWebPath, iIncFirstLine - iTotalIncLines + 1)
      iTotalIncLines = iTotalIncLines + iIncLines - 1
      
      FindInclude sFile, iPos, iLength, sIncPath
   Loop
End Sub

Function LoadFile(sFilePath)
    Dim textFile: set textFile = FileSystemObject.OpenTextFile(sFilePath)
    LoadFile = ConvertCrLf(textFile.ReadAll)
    textFile.Close
End Function

Sub SaveFile(sFilePath, sData)
   Dim textFile: Set textFile = FileSystemObject.OpenTextFile(sFilePath, 2, True) 'ForWriting, Create
   textFile.Write sData
   textFile.Close
End Sub

Function GetProfileURL(ByVal sURL)
    GetProfileURL = FileSystemObject.GetParentFolderName(sURL) & "/" &  FileSystemObject.GetBaseName(sURL) & ".profile." & FileSystemObject.GetExtensionName(sURL)
End Function

Function ConvertCrLf(ByVal sText)
   sText = Replace(sText, vbCrLf, vbCr)
   sText = Replace(sText, vbLf, vbCr)
   sText = Replace(sText, vbCr, vbCrLf)
   ConvertCrLf = sText
End Function

'Change to grab ALL <script> open and close tags, then match them up, discarding <script> tags that aren't server size, and returning just index and length of start and end tag '
Function ProfileCode(ByVal sCode, ByVal sBaseName)
  Dim i, j, bFirst

  'Get very base name of sBaseName
  i = InStr(1, sBaseName, ".")
  If i > 0 Then sBaseName = Left(sBaseName, i - 1)

  Dim codeSections: set codeSections = Server.CreateObject("System.Collections.ArrayList")

  Dim startMatches: set startMatches = ScriptStartRegex.Execute(sCode)
  Dim endMatches: set endMatches = ScriptEndRegex.Execute(sCode)

  Dim startIndex: startIndex = 0
  Dim endOffset: endOffset = 0
  Dim jLen
  j = 1
  jLen = 0

  dim section
  For StartIndex = 0 to startMatches.Count - 1
    i = startMatches(startIndex).FirstIndex + 1
    If j+jlen <> i  Then
      set section = new CodeSection
      'Response.Write "index" & startIndex & ";offset" & endOffset & ";j" & j & ";jLen" & jlen & ";i" & i & "<br>"
      section.SetLiteral j + jLen, i, sCode
      codeSections.Add section
    end If
    Do While endOffset < endMatches.Count - 1 And endMatches(startIndex + endOffset).FirstIndex + 1 < i
      endOffset = endOffset + 1
    Loop
    j = endMatches(startIndex + endOffset).FirstIndex + 1
    jLen = endMatches(startIndex + endOffset).Length
    On Error Resume Next
    set section = new CodeSection
    section.SetCode startMatches(startIndex), endMatches(startIndex + endOffset), sCode
    'If Err.Number <> 0 Then
      Response.Write "Type: " & section.SectionType & "<br>"
      Response.Write "Start:" & Server.HTMLEncode(section.StartString) & "<br>"
      Response.Write "Content: <pre style=""margin-left: 15px; background: #ffffff11"">" & Server.HTMLEncode(section.Content) & "</pre>"
      Response.WRite "End:"  & Server.HTMLEncode(section.EndString) & "<br><br>"
    'End If
    On Error goto 0
    codeSections.Add section
  Next

  Set section = new CodeSection
  section.SetLiteral j + jLen, len(sCode) + 1, sCode
  codeSections.Add section
  Dim lines: lines = 1
  Dim output: output = ""
  bFirst = true
  dim iPreviousLine: iPreviousLine = -1

  For Each Section In codeSections
    Select Case Section.Type 
      case codeLiteral
        lines = lines + Section.LineCount
        output = output & Section.Content
      case codeVBScript
        output = output & Section.StartString
        output = output & ProfileVBScriptCodeBlock(Section.Content, lines, iPreviousLine, bFirst)
        output = output & Section.EndString
        lines = lines + Section.LineCount
      case codeJScript
        output = output & Section.StartString
        output = output & ProfileJScriptCodeBlock(Section.Content, liens, iPreviousLine, bFirst)
        output = output & Section.EndString
        lines = lines + Section.LineCount
    End Select
  Next

  If bFirst Then output = output & "<" & "%" & GetProfileHeader & "%" & ">" & vbCrLf

  AddReportingFooter output

  output = Replace(output, "Response.End", "Profiler_End", 1, -1, 1)
  output = Replace(output, "Response.Redirect", "Profiler_End '", 1, -1, 1)
  output = Replace(output, "Server.Transfer", "Profiler_End '", 1, -1, 1)
  output = Replace(output, "*Profiler_End*", "Response.End", 1, -1, 1)
  output = Replace(output, sInternalCount, lines, 1, -1, 1)
  'sOut = Replace(sOut, sInternalFileName, sBaseName)
  output = Replace(output, sInternalFileName, "", 1, -1, 1)
  ProfileCode = output
End Function

Function GetProfileHeader()
   Dim sCode
   sCode = "' Intermediate file created by ASP Profiler at " & Now & ". Delete after use." & vbCrLf
   sCode = sCode & "Dim Tpr_" & sInternalFileName & "_N(" & sInternalCount & "), Tpr_" & sInternalFileName & "_T(" & sInternalCount & "), Tpr_" & sInternalFileName & "_T1" & vbCrLf
   sCode = sCode & "Function Profiler_PrintVal(value)" & vbCrLf
   sCode = sCode & "  Profiler_PrintVal = value" & vbCrLf
   sCode = sCode & "  If LEN(TRIM(CSTR(value))) = 0 Then Profiler_PrintVal = ""0""" & vbCrLf
   sCode = sCode & "End Function" & vbCrLF & vbCrLf
   sCode = sCode & "Sub Profiler_End()" & vbCrLf
   sCode = sCode & "  Response.Clear" & vbCrLf
   sCode = sCode & "  Response.ContentType = ""application/json""" & vbCrLf
   sCode = sCode & "  Response.Write ""{ """"data"""": ["" & vbCrLf" & vbCrLf
   'sCode = sCode & "Response.Write ""Line:Int|Count:Int|Time:Int|Percent|File|FileLine|Code"" & vbCrLf" & vbCrLf
   sCode = sCode & "  For Tpr_" & sInternalFileName & "_T1 = 1 To UBound(Tpr_" & sInternalFileName & "_N)" & vbCrLf
   sCode = sCode & "    Response.Write ""  ["" & Tpr__T1 & "", "" & Profiler_PrintVal(Tpr__N(Tpr__T1)) & "", "" & Profiler_PrintVal(Tpr__T(Tpr__T1)) & ""]""" & vbCrLf
   sCode = sCode & "    If Tpr__T1 <> UBound(Tpr__N) Then Response.Write  "", """ & vbCrLf
   sCode = sCode & "    Response.Write vbCrLf" & vbCrLf
   sCode = sCode & "Next" & vbCrLf
   sCode = sCode & "Response.Write ""]}""" & vbCrLf
   sCode = sCode & "*Response.End*" & vbCrLf
   sCode = sCode & "End Sub" & vbCrLf
   GetProfileHeader = sCode
End Function

Function ProfileJScriptCodeBlock(sBlock, iFirstLine, ByRef iPreviousLine, ByRef bFirst)
  'Not yet implemented. Will require reimplementing profiling code in a
  'JScript block, so it is in scope for both script and tag base vbscript/jscript
  'Default order of execution is 
  'script block for non default language (typically JScript)
  'HTML tag blocks for default language (typically VBScript)
  'script block for default language (typically vbscript)
  ProfileJScriptCodeBlock = sBlock
End Function

Function ProfileVBScriptCodeBlock(ByVal sBlock, ByVal iFirstLine, ByRef iPreviousLine, ByRef bFirst)
   Dim arrLines, i, j, sLine, iLine
   Dim bSkipLine, bSkipTime, bSkipAbsolute
   Dim bUnfinishedLine, iBeginningLine 'for "_" lines
   arrLines = Split(sBlock, vbCrLf)
   sBlock = ""
   iBeginningLine = -1
   For i = 0 To UBound(arrLines)
      sLine = Trim(Replace(arrLines(i), vbTab, ""))
      If Len(sLine) > 0 Then
         CheckLine sLine, bSkipLine, bSkipTime, bSkipAbsolute, bUnfinishedLine
         If Not bSkipAbsolute And bFirst Then
            sBlock = sBlock & GetProfileHeader
            bFirst = False
         End If
         If Not bSkipLine Then
            iLine = iFirstLine + i
            
            If bUnfinishedLine Then
               'in a "_" sequence
               If iBeginningLine < 0 Then iBeginningLine = iLine 'first
               sBlock = sBlock & ProfileCodeLine(arrLines(i), iBeginningLine, (iBeginningLine <> iLine) Or (iLine = iPreviousLine), (iBeginningLine <> iLine) Or bSkipTime, True)
            Else
               If iBeginningLine > 0 Then
                  'last line of a "_" sequence
                  sBlock = sBlock & ProfileCodeLine(arrLines(i), iBeginningLine, True, True, bSkipTime)
                  iBeginningLine = -1
               Else
                  'profile
                  sBlock = sBlock & ProfileCodeLine(arrLines(i), iLine, (iLine = iPreviousLine), bSkipTime, bSkipTime)
               End If
            End If
         Else
            'ignore
            sBlock = sBlock & arrLines(i)
         End If
      Else
         'ignore
         sBlock = sBlock & arrLines(i)
      End If
      If i < UBound(arrLines) Then sBlock = sBlock & vbCrLf
   Next
   ProfileCodeBlock = sBlock
   iPreviousLine = iLine
End Function

Function GetBeginProfileLine(ByVal iLine, ByVal bSkipCount, ByVal bSkipStartTime)
   Dim sOut
   sOut = ""
   If Not bSkipCount Then sOut = sOut & "Tpr_" & sInternalFileName & "_N(" & iLine & ") = Tpr_" & sInternalFileName & "_N(" & iLine & ") + 1: "
   If Not bSkipStartTime Then sOut = sOut & "Tpr_" & sInternalFileName & "_T1=Timer" & vbCrLf
   
   GetBeginProfileLine = sOut
End Function

Function GetProfileLine(ByVal sLine)
   If Left(Trim(Replace(sLine, vbTab, "")), 1) = "=" Then
      GetProfileLine =  "%" & "><" & "%" & sLine & "%" & "><" & "%" & vbCrLf
   Else
      GetProfileLine = sLine & vbCrLf
   End If
End Function

Function GetEndProfileLine(ByVal iLine, ByVal bSkipEndTime)
   Dim sOut
   sOut = ""
   If Not bSkipEndTime Then sOut = sOut & "Tpr_" & sInternalFileName & "_T(" & iLine & ") = Tpr_" & sInternalFileName & "_T(" & iLine & ") + 1000 * (Timer - Tpr_" & sInternalFileName & "_T1)" & vbCrLf
   
   GetEndProfileLine = sOut
End Function

Function ProfileCodeLine(ByVal sLine, ByVal iLine, ByVal bSkipCount, ByVal bSkipStartTime, ByVal bSkipEndTime)
   Dim sOut
   
   sOut = GetBeginProfileLine(iLine, bSkipCount, bSkipStartTime)
   sOut = sOut & GetProfileLine(sLine)
   sOut = sOut & GetEndProfileLine(iLine, bSkipEndTime)
   
   ProfileCodeLine = sOut
End Function

Function CountLines(sCode)
   CountLines = UBound(Split(sCode, vbCrLf)) + 1
End Function

Sub AddReportingFooter(ByRef sCode)
   sCode = sCode & "<" & "%Profiler_End%" & ">"
End Sub

Sub CheckLine(ByVal sLine, ByRef bSkipLine, ByRef bSkipTime, ByRef bSkipAbsolute, ByRef bUnfinishedLine)
   Dim i, sFirst
   
   bSkipAbsolute = False 'to skip Option Explicit etc before writing Dim.
   bUnfinishedLine = (Right(RTrim(sLine),1) = "_")
   
   '@ directives
   If Left(sLine, 1) = "@" Then '@language = vbscript etc.
      bSkipLine = True
      bSkipAbsolute = True
      Exit Sub
   End If
   
   'comments
   If Left(sLine, 1) = "'" Then 
      bSkipLine = True
      Exit Sub
   End If
   
   'block keywords
   i = InStr(1, sLine, " ")
   If i = 0 Then sFirst = sLine Else sFirst = Left(sLine, i - 1)
   sFirst = LCase(sFirst)
   Select Case sFirst
      Case "option"
         bSkipAbsolute = True
         bSkipLine = True
         Exit Sub
      Case "case", "dim", "public", "private", "const", "class", "sub", "function", "property", "static", "end"
         bSkipLine = (Not bUnfinishedLine)
         Exit Sub
      Case "select", "class"
         bSkipLine = False
         bSkipTime = True
         Exit Sub
   End Select
   
   bSkipLine = False
   bSkipTime = False
End Sub

End Class

Dim Profiler: Set Profiler = new ProfilerBase
%>