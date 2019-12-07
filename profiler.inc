<%


Const sInternalCount = "#LINECOUNT#"
Const sInternalFileName = "#FILENAME#"
Class ProfilerBase
    Private FileSystemObject

    Private Sub Class_Initialize()
        Set FileSystemObject = Server.CreateObject("Scripting.FileSystemObject")
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

Function ProfileCode(ByVal sCode, ByVal sBaseName)
   Dim i, j, bFirst, iLines, iFirstLine
   Dim sOut, iPreviousLine
   
   'Get very base name of sBaseName
   i = InStr(1, sBaseName, ".")
   If i > 0 Then sBaseName = Left(sBaseName, i - 1)

   iLines = CountLines(sCode)
   sOut = ""
   bFirst = True
   iPreviousLine = -1
   j = 1
   i = InStr(1, sCode, "<" & "%")

   Do While i > 0
      sOut = sOut & Mid(sCode, j, i - j + 2)
      j = InStr(i + 2, sCode, "%" & ">")
      If j = 0 Then
         Response.Write "Unclosed ASP tag!"
         Exit Do
      End If
      
      iFirstLine = CountLines(Left(sCode, i))
      sOut = sOut & ProfileCodeBlock(Mid(sCode, i + 2, j - i - 2), iFirstLine, iPreviousLine, bFirst)
      i = InStr(j, sCode, "<" & "%")
   Loop
   sOut = sOut & Mid(sCode, j)
   
   If bFirst Then sOut = sOut & "<" & "%" & GetProfileHeader & "%" & ">" & vbCrLf

   AddReportingFooter sOut

   sOut = Replace(sOut, "Response.End", "Profiler_End", 1, -1, 1)
   sOut = Replace(sOut, "Response.Redirect", "Profiler_End '", 1, -1, 1)
   sOut = Replace(sOut, "Server.Transfer", "Profiler_End '", 1, -1, 1)
   sOut = Replace(sOut, "*Profiler_End*", "Response.End", 1, -1, 1)
   sOut = Replace(sOut, sInternalCount, iLines, 1, -1, 1)
   'sOut = Replace(sOut, sInternalFileName, sBaseName)
   sOut = Replace(sOut, sInternalFileName, "", 1, -1, 1)
   ProfileCode = sOut
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

Function ProfileCodeBlock(ByVal sBlock, ByVal iFirstLine, ByRef iPreviousLine, ByRef bFirst)
   Dim arrLines, i, j, sLine, iLine
   Dim bSkipLine, bSkipTime, bSkipAbsolute
   Dim bUnfinishedLine, iBeginningLine 'for "_" lines
   arrLines = Split(sBlock, vbCrLf)
   sBlock = ""
   iBeginningLine = -1
   For i = 0 To UBound(arrLines)
      sLine = Trim(Replace(arrLines(i), vbTab, " "))
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
            If bUnfinishedLine Then
               Dim t, tx: t = TRIM(REPLACE(arrLines(i), vbTab, " "))
               tx = Mid(t, 1, len(t)-1)
               i = i + 1
               Do While IsLineUnfinished(arrLines(i))
                  t = TRIM(REPLACE(arrLines(i), vbTab, " "))
                  tx = tx & " " & mid(t, 1, len(t)-1)
                  i = i + 1
               Loop
               tx = tx & " " &trim(REPLACE(arrlines(i), vbTab, " "))
               sBlock = sBlock & tx
            Else
               'ignore
               sBlock = sBlock & arrLines(i)
            End If
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
   If Not bSkipEndTime Then sOut = sOut & "Tpr_" & sInternalFileName & "_T(" & iLine & ") = Tpr_" & sInternalFileName & "_T(" & iLine & ") + 1000.0 * (Timer - Tpr_" & sInternalFileName & "_T1)" & vbCrLf
   
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

Function IsLineUnfinished(line)
   IsLineUnfinished = (Right(RTrim(line),1) = "_")
End Function

Sub CheckLine(ByVal sLine, ByRef bSkipLine, ByRef bSkipTime, ByRef bSkipAbsolute, ByRef bUnfinishedLine)
   Dim i, sFirst
   
   bSkipAbsolute = False 'to skip Option Explicit etc before writing Dim.
   bUnfinishedLine = IsLineUnfinished(sLine)
   
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
         bSkipLine = True
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