SUMMARY = "Test suite using official NIST tests for FIPS 202"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"

LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302" 

SRC_URI = "file://nist_test_suite"
S = "${WORKDIR}"

# Create the `ketchup` user using useradd.bbclass
inherit useradd

USERADD_PACKAGES = "${PN}"
USERADD_PARAM:${PN} = "--shell /bin/bash --user-group --password '' ketchup"


# The user will be created before do_install is called,
# so we can use it in a chown context
HOME = "/home/ketchup"

do_install() {
    install -d ${D}${HOME}
    cp -r ${S}/nist_test_suite ${D}${HOME}

    chmod -R 755 ${D}${HOME}/nist_test_suite
    chmod 644 ${D}${HOME}/nist_test_suite/input_files/*
    chmod 644 ${D}${HOME}/nist_test_suite/response_files/*

    chown -R ketchup:ketchup ${D}${HOME}
}

INSANE_SKIP:${PN} = "ldflags"
INSANE_SKIP:${PN} = "already-stripped"

FILES:${PN} += "${HOME}"
