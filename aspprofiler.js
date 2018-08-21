
let app = new Vue({
    el: '#vueApp',
    data: {
        profileUrl: profileUrl,
        queryText: "",
        files: {},
        enableTimeMinimum: false,
        timeMinimum: 0.0,
        enableCountMinimum: false,
        countMinimum: 0,
        filterMode: "hide",
        nextFileId: 0
    },
    computed: {
      minCount() { return (this.enableCountMinimum ? this.countMinimum : -1) },
      minTime() { return (this.enableTimeMinimum ? this.timeMinimum : -1.0) },
      something() {
        return JSON.stringify({
          filter: this.filterMode || null,
          enableTime: this.enableTimeMinimum || null,
          time: this.timeMinimum || null,
          enableCount: this.enableCountMinimum || null,
          count: this.count || null
        });
      }
    },
    methods: {
        generateFile(name) {
          if(name in this.files){
            return this.files[name];
          }
          let obj = {
            "name": name,
            "lines": [],
            "count": 0,
            "time": 0,
            "countPercentage": 0,
            "timePercentage": 0,
            "display": true,
            "id": "f_" + this.nextFileId.toString()
          };
          this.nextFileId++;
          Vue.set(this.files, name, obj);
          return obj;
        },
        collapseAll() {
          for(let fileName in this.files)
            this.files[fileName].display = false;
        },
        expandAll() {
          for(let fileName in this.files)
            this.files[fileName].display = true;
        },
        getFileForLine(line){
            for(let i = 1; i < fileLimits.length; i++){
                if(fileLimits[i][0] > line)
                    return {
                        "file": this.generateFile(fileLimits[i-1][1]),
                        "offset": fileLimits[i-1][0],
                        "last": fileLimits[i][0] - 1
                    };
            }
            let tx = fileLimits[fileLimits.length - 1];
            return {
                "file": this.generateFile(tx[1]),
                "offset":tx[0],
                "last": 9999999
            };
        },
        async query() {
            this.files = {}
            let response;
            let requestUrl = this.profileUrl;
            if((this.queryText || "").length > 0)
                requestUrl += "?" + this.queryText;
            try{
                response = await axios.get(requestUrl)
            }
            catch(err){
                console.log("error encountered during run", err)
                alert("error encountered");
                return;
            }
            let data = response.data.data;
            let lineData = fileContents.map((val, line) => ({"line": line + 1,  "content": val }))
            let file = this.getFileForLine(0);
            let totalTime, totalCount;
            totalTime = totalCount = 0;
            for(let i = 0; i < lineData.length; i++){
              if(file.last < i)
                file = this.getFileForLine(i);

              let d = data[i];
              let line = lineData[d[0]-1];
              line.count = d[1];
              line.time = d[2];
              line.id = "l_" + i.toString();
              line.countPercentage = 0;
              line.timePercentage = 0;
              totalCount += d[1];
              totalTime += d[2];
              file.file.lines.push(line);
              file.file.count += d[1];
              file.file.time += d[2];
            }

            totalTime = Math.max(totalTime, 1)
            totalCount = Math.max(totalCount, 1);

            for(let fileName in this.files) {
              file = this.files[fileName];
              file.timePercentage = (file.time / totalTime) * 100.0;
              file.countPercentage = (file.count / totalCount) * 100.0;
              for(line of file.lines){
                line.timePercentage = (line.time / totalTime) * 100.0;
                line.countPercentage = (line.count / totalCount) * 100.0;
              }
            }
        }
    }
});