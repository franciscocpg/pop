AUTOBUILD_CONFIGURE_EXTRA="${AUTOBUILD_CONFIGURE_EXTRA:-} --arch=i686"
DEBDIST=precise
DEBSUFFIX=~${DEBDIST}1
source targets/debian.sh
