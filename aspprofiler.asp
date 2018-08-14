<%Option Explicit%>
<!--#include file="profiler.inc"-->
<!--#include file="aspprofiler.inc"-->
<%

Dim command: command = Request("cmd")
dim path: path = Request("path")
If Len(command) = 0 Then
  command = "new"
End If
%><!DOCTYPE HTML>
<html>
  <head>
      <meta charset="utf-8" />
      <title>ASP Profiler v2.5 - Forked 8/10/2018</title>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/normalize/8.0.0/normalize.min.css"/>
      <link rel="stylesheet" href="aspprofiler.css"/>
      <script type="text/javascript" src="https://cdnjs.cloudflare.com/ajax/libs/axios/0.18.0/axios.js"></script>
      <script type="text/javascript" src="https://cdn.jsdelivr.net/npm/vue@2.5.16/dist/vue.js"></script>
  </head>
  <body>
    <h4>ASP Profiler v2.5 - Forked 8/10/2018</h4>
      <%
      Dim cont: cont = true
      If command = "new" Then
        WritePrompt
      Else
        Dim webPath: webPath = Profiler.ToWebPath(path)
        If Len(webPath) = 0 Or Not Profiler.PathExists(webPath) Then
          Response.Write "<font color='red'>ERROR: Path not found: <b>" & path & "</b></font><br>"
          WritePrompt
        Else
          dim profilePageUrl: profilePageUrl= Profiler.GetProfileURL(webPath)
          dim code, instrumentedCode, limits
          Profiler.InstrumentCode webPath, code, instrumentedCode, limits

          If command = "create" Then
            Profiler.SaveFile Server.MapPath(profilePageUrl), instrumentedCode
            WriteProfileCreated profilePageUrl
          Else
            WriteProfileCode profilePageUrl, instrumentedCode
          End If

          WriteProfilePrompt profilePageUrl, code, limits
        End If
      End If
      %>
    <div>
      <font size=-1>
        Copyright © 2001-2015 Zafer Barutcuoglu. All Rights Reserved.<br>
        Visit <a href="http://aspprofiler.sourceforge.net/">http://aspprofiler.sourceforge.net/</a> for more information.<br>
      </font>
    </div>
  </body>
</html>
<%
Response.End
' ASP Profiler v2.5
' Copyright © 2001-2015 Zafer Barutcuoglu. All Rights Reserved.
'
' ASP Profiler is free software; you can redistribute it and/or modify
' it under the terms of the GNU General Public License as published by
' the Free Software Foundation; either version 2 of the License, or
' (at your option) any later version.
' 
' ASP Profiler is distributed in the hope that it will be useful,
' but WITHOUT ANY WARRANTY; without even the implied warranty of
' MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
' GNU General Public License for more details.
' 
' You should have received a copy of the GNU General Public License
' along with ASP Profiler; if not, write to the Free Software
' Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
' 
' Visit http://aspprofiler.sourceforge.net/ for more information.
'
' - 20010507 Created.
' - 20120424 CInt -> CLng (thanks to Diane Connors).
' - 20130813 Fixed header bug, added newline conversion (thanks to Tomas Wallentinus).
' - 20150209 Added meta tag for IE10+ (thanks to Peter Chr. Gram).
' - 20181008 Refactored and modified to use Vue/Javascript on client side instead of VBScript
%>