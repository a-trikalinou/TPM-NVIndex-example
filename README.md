# TPM-NVIndex-example

## Prerequisites
```sudo apt-get install libtss2-dev tpm2-tools```

## Compile
```gcc -o tpm_nv_example tpm_nv_example.c -ltss2-esys -ltss2-sys -ltss2-tctildr```

## Run
```sudo ./tpm_nv_example```

## Credits
Code modified from https://gist.github.com/williamcroberts/e08c7a99fa7a66be7c30bd62e38d4e24 by William Roberts
