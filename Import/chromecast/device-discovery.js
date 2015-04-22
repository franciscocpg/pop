var Mdns = require('./node_modules/airplay2/node_modules/mdns-js');
var Device = require( './node_modules/airplay2/airplay/device' ).Device;

var superagent = require('superagent');
var xml2json = require('xml2json');
/**/
var mdns = new Mdns("airplay");
console.log('starting airplay mDns...');
mdns.on('ready', function () {
    try{
        console.log('airplay mDns ready, discovering devices...');
        mdns.discover();
    }catch(e){
        console.error('Error on airplay mdns discover: ', e);
    }
});
mdns.on( 'update', function() {
    try{
        console.log('Receiving airplay mDns discovering results...');
        var info = mdns.ips('_airplay._tcp');
        if( info && info.length > 0)
        {	for(var i = 0; i < info.length; i++){
            var index = i;
            console.log('found airplay', info[index]);
            var device = new Device( index, [info[index]] );
            device.on( 'ready', function( d ) {
                try{
                    var desc = JSON.stringify({deviceType:'airplay', name:'Apple TV', ip: info[index], url:info[index]+':' + 7000  });
                    console.log('<<DEVICE_FOUND: %s>>||\n', desc);
                }catch(e){
                    console.error('Error on airplay device request: ', e);
                }
            });
        }
        }else
        {
            console.log('airplay not found');
        }
    }catch(e){
        console.error('Error on airplay mdns request: ', e);
    }
});
var mdns_g = new Mdns("googlecast");
console.log('starting googlecast mDns...');
mdns_g.on('ready', function () {
    try{
        console.log('googlecast mDns ready, discovering devices...');
        mdns_g.discover();
    }catch(e){
        console.error('Error on googlecast mdns discover: ', e);
    }
});
mdns_g.on( 'update', function() {
    try{
        console.log('Receiving googlecast mDns discovering results...');
        var info = mdns_g.ips('_googlecast._tcp');
        if( info && info.length > 0)
        {
            for(var i = 0; i < info.length; i++){
                var index = i;
                console.log('found google device', info[index]);
                var ssdnUrl = 'http://'+info[index]+':8008/ssdp/device-desc.xml';
                console.log('ssdp url: ', info[index]);
                var request = superagent.get(ssdnUrl);
                request.buffer();
                request.type('xml');
                request.end(function(err, res) {
                    if (err)
                    {
                        console.error('Error on ssdp request: ', err);
                        return;
                    }
                    console.log('ssdp response: ', res.text);
                    var parsedBody = xml2json.toJson(res.text, {object: true});
                    var desc = JSON.stringify({deviceType:'googlecast', name:parsedBody.root.device.friendlyName, ip: info[index], url:parsedBody.root.URLBase +':'+8009  });
                    console.log('<<DEVICE_FOUND: %s>>||\n', desc);
                });
            }
        }else
        {
            console.log('googlecast not found');
        }
    }catch(e){
        console.error('Error on google mdns request ', e);
    }
});
//setTimeout(function(){  mdns.shutdown(); mdns_g.shutdown();}, 5000);
