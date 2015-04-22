var _ = require('lodash');
var util = require('util');
var net_utils = require('./net-utils');
var CastClient = require('castv2-client').Client;
var DefaultMediaReceiver = require('castv2-client').DefaultMediaReceiver;
var listen_port = null;
var deviceIP = null;
var client = null;
var sessionPlayer = null;

var PrintWarn = function (msg) {
    msg = (msg && typeof msg != 'string' && msg.toString().indexOf('[') == 0) ? JSON.stringify(msg) : msg;
    console.log('<<WARN: %s}>>||\n', msg);
}
var PrintError = function (msg) {
    msg = (msg && typeof msg != 'string' && msg.toString().indexOf('[') == 0) ? JSON.stringify(msg) : msg;
    msg = !msg || msg == '' ? '{}' : msg;
    console.error('<<ERR: %s>>||\n', msg);
}
var PrintMediaStatus = function (msg) {
    msg = (msg && typeof msg != 'string' && msg.toString().indexOf('[') == 0) ? JSON.stringify(msg) : msg;
    msg = !msg || msg == '' ? '{}' : msg;
    console.log('<<MSTAT: %s>>||\n', msg);
}
var PrintDeviceStatus = function (msg) {
    var status = (msg && msg.applications && _.find(msg.applications, {appId: DefaultMediaReceiver.APP_ID})) ? 'CONNECTED' : 'STOPPED';
    //var status = (msg && typeof msg != 'string' && msg.toString().indexOf('[') == 0) ? JSON.stringify(msg) : msg;
    console.log('<<DSTAT: %s>>||\n', status);
}
var standardWarnPrint = function (err, status) {
    if (err) {
        PrintWarn(err);
    }
    PrintMediaStatus(status);
}
var standardWarnDevicePrint = function (err, status) {
    if (err) {
        PrintWarn(err);
    }
    PrintDeviceStatus(status);
}
var standardPrint = function (err, status) {
    if (err) {
        PrintError(err);
    }
    PrintMediaStatus(status);
}
function CommandProcessor() {
}
CommandProcessor.prototype.deviceType = 'googlecast';
CommandProcessor.prototype.processCommand = function (url_parts) {
    try {
        var query = url_parts.query;
        if (url_parts.path.indexOf('/load') != -1) {

            client = new CastClient();
            client.on('error', function (err) {
                PrintError(err);
                //  client.close();
            });

            client.on('status', function (status) {
                PrintDeviceStatus(status);
            });

            client.on('close', function () {
                sessionPlayer = null;
                client = null;
            });
            deviceIP = query['deviceIP'];
            console.log('connecting ...');
            client.connect(deviceIP, function () {
                    console.log('connected, launching app ...');
                    client.launch(DefaultMediaReceiver, function (err, player) {

                        if (err) {
						     console.log('launching stoped with error ...');
                            PrintError(err);
                            client.close();
                            client = null;
                            return;
                        }
                        sessionPlayer = player;

                        var torrentFile = query['torrentFile'];
                        if (torrentFile.indexOf('http') != 0) {
                            var listen_url = net_utils.getMatchedLocalIp(deviceIP);
                            torrentFile = util.format('http://%s:%s/getStream/movie.mp4?torrentFile=%s', listen_url, listen_port, encodeURIComponent(query['torrentFile']));
                        }

                        var media = {
                            // Here you can plug an URL to any mp4, webm, mp3 or jpg file.

                            contentId: torrentFile
                        };
                        sessionPlayer.on('status', function (status) {
                            PrintMediaStatus(status);
                        });

                        console.log('app "%s" launched, loading media %s ...', sessionPlayer.session.displayName, media.contentId);

                        sessionPlayer.load(media, { autoplay: true }, function (err, status) {
                            standardPrint(err, status);
                            if (query['volume'])
                                client.setVolumeMute(parseFloat(query['volume']), query['muted'].indexOf('1') != -1, standardWarnDevicePrint);
                        });
                    });
                }
            );
        }
        else if (url_parts.path.indexOf('/play') != -1) {

            if (!client || !sessionPlayer) return module.exports.noSession;
            //if (isPaused()) {
            sessionPlayer.play(standardWarnPrint);
            // }
        } else if (url_parts.path.indexOf('/pause') != -1) {

            if (!client || !sessionPlayer) return module.exports.noSession;

            //if (isPlaing()) {
            sessionPlayer.pause(standardWarnPrint);
            // }
        } else if (url_parts.path.indexOf('/stop') != -1) {

            if (!client || !sessionPlayer) return module.exports.noSession;
            // sessionPlayer.stop(standardWarnPrint);
            client.stop(sessionPlayer, function (err, status) {
                standardWarnDevicePrint(err, status);
                //   sessionPlayer.close();
                sessionPlayer = null;
                client = null;
            });

        } else if (url_parts.path.indexOf('/mute') != -1) {

            if (!client || !sessionPlayer) return module.exports.noSession;

            client.setMute(query['muted'].indexOf('1') != -1, standardWarnDevicePrint);

        } else if (url_parts.path.indexOf('/volume') != -1) {

            client.setVolume(parseFloat(query['volume']), standardWarnDevicePrint);

        } else if (url_parts.path.indexOf('/seek') != -1) {

            if (!client || !sessionPlayer) return module.exports.noSession;
            sessionPlayer.seek(parseFloat(query['seek']), standardWarnPrint);

        } else if (url_parts.path.indexOf('/deviceState') != -1) {

            if (!client) {
                PrintDeviceStatus(null);
                return module.exports.notInited;
            }

            client.getStatus(standardWarnDevicePrint);

        } else if (url_parts.path.indexOf('/mediaState') != -1) {

            if (!client || !sessionPlayer) {
                PrintMediaStatus(null);
                return module.exports.noSession;
            }

            sessionPlayer.getStatus(standardWarnPrint);
        }
        return module.exports.success;
    } catch (e) {
        PrintError(e)
        return module.exports.generalError;
    }
}
CommandProcessor.prototype.close = function()
{
    try {
        if (!client || !sessionPlayer) return;
        // sessionPlayer.stop(standardWarnPrint);
        client.stop(sessionPlayer, function (err, status) {
            standardWarnDevicePrint(err, status);
            //   sessionPlayer.close();
            sessionPlayer = null;
            client = null;
        });
    }catch(e){
        console.error('client.close error',e);
    }
}
module.exports.success = 'OK';
module.exports.notInited = 'Not Initialized';
module.exports.noSession = 'Not session found';
module.exports.generalError = 'Exception raised';
module.exports.Create = function (aListen_port) {
    listen_port = aListen_port;
    return new CommandProcessor();
};