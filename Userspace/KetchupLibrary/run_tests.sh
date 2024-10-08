#!/bin/sh

INFILES_DIR="./nist_tests/input_files"
OUTFILES_DIR="./nist_tests/output_files"
RESPONSE_DIR="./nist_tests/response_files"

test_file() {
    echo -n "Checking $1..."
    sed "/^#/d" "$RESPONSE_DIR/$1" | diff "$OUTFILES_DIR/$1" - > /dev/null

    if [ $? -eq 0 ] ; then
        echo -e "\t\tPassed!"
    else
        echo -e "\t\tNot Passed!"
    fi

}

mkdir -p $OUTFILES_DIR

./nist_tests.out $INFILES_DIR $OUTFILES_DIR


for file in $(ls $OUTFILES_DIR) ; do
    test_file $file
done

rm -r $OUTFILES_DIR
