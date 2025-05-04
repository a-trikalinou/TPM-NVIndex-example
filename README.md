# TPM-NVIndex-example

## Prerequisites
```sudo apt-get install libtss2-dev tpm2-tools```

## Compile
```gcc -o tpm_nv_example tpm_nv_example.c -ltss2-esys -ltss2-sys -ltss2-tctildr```

## Run
```sudo ./tpm_nv_example```

