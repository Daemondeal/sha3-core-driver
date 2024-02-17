SUMMARY = "An application to compute SHA3 hashes, using OpenSSL as a backend" 
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://sha3hash-openssl"

S = "${WORKDIR}"

do_install() {
    install -d ${D}/${bindir}
    install -m 0755 ${S}/sha3hash-openssl ${D}/${bindir}
}

INSANE_SKIP:${PN} = "ldflags"
INSANE_SKIP:${PN} = "already-stripped"
