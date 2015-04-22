var fs = require("fs");
var url = require('url');

var net_utils = require('./net-utils');
var ip = net_utils.getLocalIp();
var commandProcessor = null;
var http = require('http');
var listen_url = null;
var listen_port = null;

var vidStreamer = require('./vid-streamer');
var server = http.createServer(function (request, response) {
    try {
        if (request.url.indexOf('/getStream/') != -1) {
            vidStreamer(request, response);
        } else if (request.url.indexOf('/getSubtitle/') != -1) {
            response.writeHead(200, {'Content-Type': 'text/plain'});
            response.end('Hello getSubtitle\n');
        } else if (request.url.indexOf('/command/') != -1) {

            commandProcessor = getCmdProcessor(url.parse(request.url, true));

            if(request.url.indexOf('/command/stopserver')!= -1) {
                server.close();
                response.writeHead(200, {'Content-Type': 'text/plain'});
                response.end(JSON.stringify(commandProcessor.successResult) );
            }

            var result = commandProcessor.processCommand( url.parse(request.url, true));

            response.writeHead(200, {'Content-Type': 'text/plain'});
            response.end(result);
        } else {
            response.writeHead(404, {'Content-Type': 'text/plain'});
            response.end('404 - File not found\n');
        }
    } catch (e) {
        console.error(e);
        response.writeHead(500, {'Content-Type': 'text/plain'});
        response.end('500 - server error\n');
    }

});
server.listen(0, function () {
    listen_url = 'http://' + ip + ':' + server.address().port + '/';
    listen_port = server.address().port;
    console.log('<<LISTEN_URL: %s>>||',listen_url);
});
var getCmdProcessor = function(url_parts){
    var deviceType = url_parts.query['deviceType'];
    if(!deviceType && !commandProcessor)
    {
        console.error('<<ERR: %s>>||', 'Not inited');
        return commandProcessor;
    }
    if(deviceType) {
        if (commandProcessor) {

            if (commandProcessor == deviceType)
                return commandProcessor;
            else {
                commandProcessor.close();
                commandProcessor= null;
            }
        }
        if(deviceType == 'googlecast'){
            return require('./command-processor').Create(listen_port);
        }else{
            if(deviceType == 'airplay'){
                return require('./air-command-processor').Create(listen_port);
            }else{
                console.error('<<ERR: %s>>||', 'Unknown deviceType:'+deviceType);
                return commandProcessor;
            }
        }

    }else
    {
        return commandProcessor;
    }
}

