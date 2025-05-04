#include <stdio.h>
#include <string.h>
#include <tss2/tss2_esys.h>
#include <tss2/tss2_rc.h>

#define DATA "hello world"

int main(int argc, char *argv[]) {

	ESYS_CONTEXT *ectx = NULL;

	ESYS_TR nv_index = 0;
	ESYS_TR nv_index2 = 0;

	int rc = 1;

	/*
	 * create a connection to the TPM letting ESAPI choose how to get there.
	 * If you need more control, you can use tcti and tcti-ldr libraries to
	 * get a TCTI pointer to use for the tcti argument of Esys_Initialize.
	 */
	rc = Esys_Initialize(&ectx,
			NULL, /* let it find the TCTI */
			NULL);/* Use whatever ABI */
	if (rc != TSS2_RC_SUCCESS) {
		fprintf(stderr, "Esys_Initialize: 0x%x\n", rc);
		return 1;
	}

	/* build a template for the NV index */
	TPM2B_NV_PUBLIC pub_templ = {
		/* this is counter intuitive, but it tells the TSS2 library to calculate this for us */
		.size = 0,
		/* The things that define what NV index we are creating */
		.nvPublic = {
			/* uses sha256 to identify the tpm object by name */
			.nameAlg = TPM2_ALG_SHA256,
			/* allows the owner password or index password r/w access */
			.attributes = TPMA_NV_OWNERWRITE |
				TPMA_NV_OWNERREAD            |
				TPMA_NV_AUTHWRITE            |
				TPMA_NV_AUTHREAD             |
                /* NVIndex is extend-only */
                (0x4 << TPMA_NV_TPM2_NT_SHIFT) & TPMA_NV_TPM2_NT_MASK,
			/* can hold 32 bytes of data */
			.dataSize = 32,
			/* Create at NV Index 1 or address 0x1000001 */
			.nvIndex = TPM2_HR_NV_INDEX + 1
		},
	};

	/* Define the space and store the nv_index for future use
     * Note, this is for debug purposes. The NVIndex will be
     * defined by OpenHCL. */
	rc = Esys_NV_DefineSpace(
	    ectx,
		ESYS_TR_RH_OWNER, /* create an NV index in the owner hierarchy */
	    ESYS_TR_PASSWORD, /* auth as the owner with a password, which is empty */
	    ESYS_TR_NONE,
	    ESYS_TR_NONE,
	    NULL,
	    &pub_templ,
	    &nv_index);
	if (rc != TSS2_RC_SUCCESS) {
		fprintf(stderr, "Esys_NV_DefineSpace: 0x%x\n", rc);
		goto out;
	}

	/* Note if you need to set the owner hierarchy auth value you use:
	 * TSS2_RC Esys_TR_SetAuth(
	 *     ESYS_CONTEXT *esysContext,
     *     ESYS_TR handle,
     *     TPM2B_AUTH const *authValue);
	 */

	printf("Created NV Index: 0x%x\n", pub_templ.nvPublic.nvIndex);
	printf("ESYS_TR: 0x%x\n", nv_index);

	/*
	 * Note: if you need to convert the nvIndex into an ESYS_TR, the below will
	 * do it. You can subsequently use the ESYS_TR2 handle to read/extend the
     * NV index.
	 */
	rc = Esys_TR_FromTPMPublic(
		ectx,
		TPM2_HR_NV_INDEX + 1,
		ESYS_TR_NONE,
		ESYS_TR_NONE,
		ESYS_TR_NONE,
		&nv_index2);
	if (rc != TSS2_RC_SUCCESS) {
		fprintf(stderr, "Esys_TR_FromTPMPublic: 0x%x\n", rc);
		goto out;
	}

	printf("ESYS_TR2: 0x%x\n", nv_index2);

    /* Find NVIndex public information */
    TPM2B_NV_PUBLIC *nv_public = NULL;
    TPM2B_NAME *nv_name = NULL;

    rc = Esys_NV_ReadPublic(
        ectx,
        nv_index2,        /* Handle to the NV index */
        ESYS_TR_NONE,
        ESYS_TR_NONE,
        ESYS_TR_NONE,
        &nv_public,      /* Output: Public information of the NV index */
        &nv_name         /* Output: Name of the NV index */
    );

    if (rc != TSS2_RC_SUCCESS) {
        fprintf(stderr, "Esys_NV_ReadPublic: 0x%x\n", rc);
        goto out;
    }

    printf("NV Index Size: %u bytes\n", nv_public->nvPublic.dataSize);

	/* copy some data into a buffer to send to the TPM */
	TPM2B_MAX_NV_BUFFER write_data = { 0 };
	memcpy(write_data.buffer, DATA, sizeof(DATA));
	write_data.size = sizeof(DATA);

	/*
	 * Write the data to the TPM NV index at offset 0.
     * Cannot be used for extend-only NVIndex.
	 */
	// rc = Esys_NV_Write(
	//     ectx,
	//     nv_index, /* authenticate to the NV index using the NV index password */
	//     nv_index, /* the nv index to write to */
	//     ESYS_TR_PASSWORD,
	// 	ESYS_TR_NONE,
	// 	ESYS_TR_NONE,
	//     &write_data,
	//     0);
	// if (rc != TSS2_RC_SUCCESS) {
	// 	fprintf(stderr, "Esys_NV_Write: 0x%x\n", rc);
	// 	goto out;
	// }

    /*
	 * Extend TPM NV index
	 */
    rc = Esys_NV_Extend(
        ectx,
        nv_index2, /* authenticate to the NV index using the NV index password */
        nv_index2, /* the nv index to extend */
        ESYS_TR_PASSWORD,
        ESYS_TR_NONE,
        ESYS_TR_NONE,
        &write_data);
    if (rc != TSS2_RC_SUCCESS) {
        fprintf(stderr, "Esys_NV_Extend: 0x%x\n", rc);
        goto out;
    }

	/* Read the NVIndex contents */
	TPM2B_MAX_NV_BUFFER *read_data = NULL;
	rc = Esys_NV_Read(
	    ectx,
	    nv_index2, /* authenticate to the NV index using the NV index password */
		nv_index2, /* the nv index to read from */
	    ESYS_TR_PASSWORD,
	    ESYS_TR_NONE,
	    ESYS_TR_NONE,
        nv_public->nvPublic.dataSize,
	    0,
	    &read_data);
	if (rc != TSS2_RC_SUCCESS) {
		fprintf(stderr, "Esys_NV_Read: 0x%x\n", rc);
		goto out;
	}

	/* Print NVIndex data */
    for (size_t i = 0; i < read_data->size; i++) {
        printf("%02x", read_data->buffer[i]);
    }
    printf("\n");

    Esys_Free(read_data);
    Esys_Free(nv_name);
    Esys_Free(nv_public);

	/* success */
	rc = 0;

out:

	/* remove the NV space */
	if (nv_index) {
		int rc2 = Esys_NV_UndefineSpace(
			ectx,
			ESYS_TR_RH_OWNER,
			nv_index,
			ESYS_TR_PASSWORD,
			ESYS_TR_NONE,
			ESYS_TR_NONE);
		if (rc2 != TSS2_RC_SUCCESS) {
			fprintf(stderr, "Esys_NV_UndefineSpace: 0x%x\n", rc2);
			rc = 1;
		}
	}

	Esys_Finalize(&ectx);

	return rc;
}
