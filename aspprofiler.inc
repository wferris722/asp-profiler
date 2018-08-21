<%

Sub WritePrompt()
%>
    <form method="post" name="frmProfile">
      <input type="hidden" name="cmd" value="">
      <b>URL:</b> http://<%=Request.ServerVariables("SERVER_NAME")%>/ <input type="text" name="path" value="index.asp"><br>
      <br>
      <input type="button" title="Will automatically create a new filename.profile.asp file file for profiling" value="Create Intermediate File" onclick="this.form.cmd.value='create';this.form.submit();"> if your server has Write access from within ASP files.<br>
      <input type="button" title="Will generate source to put into a new filename.profile.asp file for subsequent profiling" value="View Intermediate Source" onclick="this.form.cmd.value='view';this.form.submit();"> if that fails, to save and upload the file yourself.<br>
   </form>
<%
End Sub

Sub WriteProfileCreated(profileUrl)
%>
    <font size=+1>Created profile file: <b><%=profileURL%></b></br></font>
<%
End Sub

Sub WriteProfileCode(profileUrl, profileCode)
%>
    <font size=+1>Please copy the following source and save/upload it as:</br>https://<%=Request.ServerVariables("SERVER_NAME")%><b><%=profileUrl%></b> <u>before</u> you click "Run" below</font></br>
    <textarea id="instrumentedCode" cols="60" rows="10" wrap="off"><%=Server.HTMLEncode(profileCode)%></textarea><br>
    <input type="button" value="Copy to Clipboard" onclick="copyToClipboard()"/><br>
    <script type="text/javascript">
    function copyToClipboard() {
      document.getElementById("instrumentedCode").select();
      document.execCommand("copy");
    }
    </script>
<%
End Sub

Sub WriteProfilePrompt(profileUrl, code, limits)
%>
    <div id="vueApp">
        <span>{{ something }}</span>
        <div class="filter_controls">
            <div class="control_span">
              <span class="control_label">Display:</span>
              <input type="radio" v-model="filterMode" value="dim">dim</input>
              <input type="radio" v-model="filterMode" value="hide">hide</input>
            </div>
            <div class="control_span">
              <span class="control_label">Min Time:</span>
              <input type="checkbox" v-model="enableTimeMinimum"><input type="number" :disabled="!enableTimeMinimum" v-model.number="timeMinimum"/>ms
            </div>
            <div class="control_span">
              <span class="control_label">Min Count:</span>
              <input type="checkbox" v-model="enableCountMinimum"><input type="number" :disabled="!enableCountMinimum" v-model.number="countMinimum"/>ms
            </div>
        </div>
        <div>
          <span><b>{{profileUrl}}</b>?<input type="text" :value="queryText"/><br>
          <button @click="query()">Profile Code</button>
        </div>
        <div>
          <button @click="collapseAll()">Collapse All</button>
          <button @click="expandAll()">Expand All</button>
        </div>
        <div class="fileView" v-for="file in files" :key="file.id" :class="{ 'file_dim': filterMode == 'dim' && (file.count < minCount || file.time < minTime), 'file_hide': filterMode == 'hide' && (file.count < minCount || file.time < minTime) }">
          <div class="fileName">
            <button class="button" @click="file.display = !file.display">{{ file.display ? "-" : "+" }}</button>
            <span class="file_stat file_count">{{ file.count }} ({{ file.countPercentage.toFixed(2) }}%)</span>
            <span class="file_stat file_time">{{ file.time }}ms ({{ file.timePercentage.toFixed(2) }}%)</span>
            <span class="file_name">{{ file.name }}</span>
          </div>
          <div class="lineView" :class='{"hidden" : !file.display}'>
            <div class="line line_header">
              <span class="line_stat line_number">Line</span>
              <span class="line_stat line_count">Count</span>
              <span class="line_stat line_time">Time</span>
            </div>
            <div class="line"
              :class="{ 'line_dim' : filterMode == 'dim' && (line.count < minCount || line.time < minTime), 'line_hide': filterMode == 'hide' && (line.count < minCount || line.time < minTime) }" 
              v-for="(line, index) in file.lines" :key="line.id">
              <span class="line_stat line_number">{{ index + 1 }}</span>
              <span class="line_stat line_count">{{ line.count }} ({{ line.countPercentage.toFixed(2) }}%)</span>
              <span class="line_stat line_time">{{ line.time }} ({{ line.timePercentage.toFixed(2) }}%)</span>
              <pre class="line_data">{{ line.content }}</pre>
            </div>
          </div>
        </div>
    </div>
    <script type="text/javascript">
      var fileContents = [
      <%
      Dim index, fileContents: fileContents = Split(code, vbCrLf)
      For index = 0 to UBound(fileContents)
        Response.Write "  " & Profiler.JSONString(fileContents(index)) & ", " & vbCrLf
      Next
      %>
      ];
      var profileUrl = <%=Profiler.JSONString(profileUrl)%>
      var fileLimits = [
      <%
      Dim itm
      For Each itm in limits.Items
        Response.Write "  [ " & itm(0) & ", " & Profiler.JSONString(itm(1)) & ", " & itm(2) & "], " & vbCrLf        
      Next
      %>
      ];
      Object.freeze(fileContents);
      Object.freeze(fileLimits);
    </script>
    <script type="text/javascript" src="aspprofiler.js"></script>
    
<%
End Sub

%>