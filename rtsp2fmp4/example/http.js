var http = require('http');
var path = require("path") ;
var fs = require('fs');
var httpObj = http.createServer(function(req,res){
	res.setHeader("Access-Control-Allow-Origin","*");
    var url = req.url=='/'?'index.html':req.url;
	console.log(url)
	
	if (url.substring(url.length - 4, 4) == ".mp4"){
		//res.set('Content-Type', 'application/octet-stream');
	}
    fs.readFile("." + url,'binary',function(err,data){
        if(err){ 
            res.write('404,您访问的页面不存在');
            res.end();    
        }else{
            res.write(data,'binary');
            res.end();    
        }
    });
});

httpObj.listen(9080);