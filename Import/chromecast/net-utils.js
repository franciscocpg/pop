function net_utils() {
}

net_utils.prototype.getLocalIp = function(){
    var os=require('os');
    var ifaces=os.networkInterfaces();
    var ip = null;
    for (var dev in ifaces) {
        var alias=0;
        ifaces[dev].forEach(function(details){
            if (details.family=='IPv4' && details.internal == false) {
                ++alias;
                ip =  details.address;
            }
        });
    }
    return ip;
};
net_utils.prototype.getMatchedLocalIp = function(addr){
    var os=require('os');
    var addrs = addr.split('.');
    var locAdrr = null;
    var ifaces=os.networkInterfaces();
    var ip = null;
    var rate = 0;
    var max_rate = 0;
    for (var dev in ifaces) {
        var alias=0;
        ifaces[dev].forEach(function(details){
            if (details.family=='IPv4' && details.internal == false) {
               rate=0;
                locAdrr = details.address.split('.');
               for(var i = 0; i < addrs.length; i++)
                {
                    rate +=  locAdrr[i] == addrs[i] ? 1 :0;
                }
                if (max_rate < rate)
                {
                    max_rate = rate;
                    ip = details.address;
                }
            }
        });
    }
    return ip;
};
module.exports = new net_utils();