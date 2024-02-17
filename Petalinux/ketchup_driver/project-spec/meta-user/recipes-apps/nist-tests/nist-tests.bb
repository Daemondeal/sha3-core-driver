SUMMARY = "Test suite using official NIST tests for FIPS 202"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"

LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302" 

SRC_URI = "file://tests"

S = "${WORKDIR}"

HOME = "/home/petalinux"


do_install() {
    install -d ${D}${HOME}
    cp -r ${S}/tests ${D}${HOME}

    # NOTE: it would be better if we could give proper
    #       permissions and change the user, but until
    #       we figure out how to change user properly
    #       this should be fine.
    chmod -R 777 ${D}${HOME}/tests
    chmod 777 ${D}${HOME}/tests/nist_tests
    chmod 777 ${D}${HOME}/tests/run_tests.sh
}

INSANE_SKIP:${PN} = "ldflags"
INSANE_SKIP:${PN} = "already-stripped"

FILES:${PN} = "${HOME}"
