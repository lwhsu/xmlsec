/** 
 * XMLSec library
 *
 * Ciphers
 *
 * See Copyright for the status of this software.
 * 
 * Author: Aleksey Sanin <aleksey@aleksey.com>
 */
#include "globals.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/rand.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>

#include <xmlsec/xmlsec.h>
#include <xmlsec/keys.h>
#include <xmlsec/transforms.h>
#include <xmlsec/transformsInternal.h>
#include <xmlsec/errors.h>
#include <xmlsec/openssl/evp.h>

/**********************************************************************
 *
 * EVP Cipher methods
 *
 *********************************************************************/
/**
 * xmlSecOpenSSLEvpCipherGenerateIv:
 * @cipher: the pointer to EVP_* cipher transform. 
 *
 * Generates IV vector.
 *
 * Returns the number of bytes in IV vector or a negative value
 * if an error occurs.
 */
int 	
xmlSecOpenSSLEvpCipherGenerateIv(xmlSecCipherTransformPtr cipher) {
    int ret;

    xmlSecAssert2(cipher != NULL, -1);
    
    if(!xmlSecBinTransformCheckSubType(cipher, xmlSecBinTransformSubTypeCipher) ||
       (cipher->cipherData == NULL)) {
	xmlSecError(XMLSEC_ERRORS_HERE,
		    XMLSEC_ERRORS_R_INVALID_TRANSFORM,
		    "xmlSecBinTransformSubTypeCipher");
	return(-1);
    }


    xmlSecAssert2(cipher->iv != NULL, -1);

    /* generate random iv */
    ret = RAND_bytes(cipher->iv, cipher->id->ivSize);
    if(ret != 1) {
        xmlSecError(XMLSEC_ERRORS_HERE,
		    XMLSEC_ERRORS_R_CRYPTO_FAILED,
		    "RAND_bytes - %d", ret);
	return(-1);    
    }
    
    return(cipher->id->ivSize);
}

/**
 * xmlSecOpenSSLEvpCipherInit:
 * @cipher: the pointer to EVP_* cipher transform. 
 *
 * Initialize encryption/decryption.
 *
 * Returns the number of bytes processed or a negative value
 * if an error occurs.
 */
int 	
xmlSecOpenSSLEvpCipherInit(xmlSecCipherTransformPtr cipher) {
    xmlSecOpenSSLEvpCipherTransformPtr evpCipher;
    int ret;

    xmlSecAssert2(cipher != NULL, -1);
    
    if(!xmlSecBinTransformCheckSubType(cipher, xmlSecBinTransformSubTypeCipher) ||
       (cipher->cipherData == NULL)) {
	xmlSecError(XMLSEC_ERRORS_HERE,
		    XMLSEC_ERRORS_R_INVALID_TRANSFORM,
		    "xmlSecBinTransformSubTypeCipher");
	return(-1);
    }
    
    evpCipher = (xmlSecOpenSSLEvpCipherTransformPtr)cipher;
    if(cipher->encode) {
	ret = EVP_EncryptInit(&(evpCipher->cipherCtx), NULL, NULL, cipher->iv); 		
	if(ret != 1) {
	    xmlSecError(XMLSEC_ERRORS_HERE,
			XMLSEC_ERRORS_R_CRYPTO_FAILED,
			"EVP_EncryptInit - %d", ret);
	    return(-1);    
	}
    } else {
	ret = EVP_DecryptInit(&(evpCipher->cipherCtx), NULL, NULL, cipher->iv);
	if(ret != 1) {
	    xmlSecError(XMLSEC_ERRORS_HERE,
			XMLSEC_ERRORS_R_CRYPTO_FAILED,
			"EVP_DecryptInit - %d", ret);
	    return(-1);    
	}
    }	    
    return(0);
}

/**
 * xmlSecOpenSSLEvpCipherUpdate:
 * @cipher: the pointer to EVP_* cipher transform. 
 * @buffer: the input buffer.
 * @size: the input buffer size.
 *
 * Encrypts/decrypts new piece of data.
 *
 * Returns the number of bytes processed or a negative value
 * if an error occurs.
 */
int 	
xmlSecOpenSSLEvpCipherUpdate(xmlSecCipherTransformPtr cipher,
			 const unsigned char *buffer, size_t size) {
    xmlSecOpenSSLEvpCipherTransformPtr evpCipher;
    int res;
    int ret;

    xmlSecAssert2(cipher != NULL, -1);
    xmlSecAssert2(buffer != NULL, -1);
    xmlSecAssert2(size > 0, -1);
    
    if(!xmlSecBinTransformCheckSubType(cipher, xmlSecBinTransformSubTypeCipher) ||
       (cipher->cipherData == NULL)) {
	xmlSecError(XMLSEC_ERRORS_HERE,
		    XMLSEC_ERRORS_R_INVALID_TRANSFORM,
		    "xmlSecBinTransformSubTypeCipher");
	return(-1);
    }

    evpCipher = (xmlSecOpenSSLEvpCipherTransformPtr)cipher;
    res = cipher->id->bufOutSize;
    if(cipher->encode) {	
	ret = EVP_EncryptUpdate(&(evpCipher->cipherCtx), 
		cipher->bufOut, &res, (unsigned char *)buffer, size);    		 		 
    } else {
	ret = EVP_DecryptUpdate(&(evpCipher->cipherCtx), 
		cipher->bufOut, &res, (unsigned char *)buffer, size);    		 		 
    }
    if(ret != 1) {
	xmlSecError(XMLSEC_ERRORS_HERE,
		    XMLSEC_ERRORS_R_CRYPTO_FAILED,
		    (cipher->encode) ? "EVP_EncryptUpdate - %d" :  "EVP_DecryptUpdate - %d", ret);
	return(-1);	
    }
    return(res);    
}

/**
 * xmlSecOpenSSLEvpCipherFinal:
 * @cipher: the pointer to EVP_* cipher transform. 
 *
 * Finalize encryption/decryption.
 *
 * Returns the number of bytes processed or a negative value
 * if an error occurs.
 */
int 	
xmlSecOpenSSLEvpCipherFinal(xmlSecCipherTransformPtr cipher) {
    xmlSecOpenSSLEvpCipherTransformPtr evpCipher;
    int res;
    int ret;

    xmlSecAssert2(cipher != NULL, -1);
    
    if(!xmlSecBinTransformCheckSubType(cipher, xmlSecBinTransformSubTypeCipher) ||
        (cipher->cipherData == NULL)) {
	xmlSecError(XMLSEC_ERRORS_HERE,
		    XMLSEC_ERRORS_R_INVALID_TRANSFORM,
		    "xmlSecBinTransformSubTypeCipher");
	return(-1);
    }

    evpCipher = (xmlSecOpenSSLEvpCipherTransformPtr)cipher;
    res = cipher->id->bufOutSize;
    if(cipher->encode) {	
	ret = EVP_EncryptFinal(&(evpCipher->cipherCtx), 
		cipher->bufOut, &res);    		 		 
    } else {
	int b;
	/*
	 * The padding used in XML Enc does not follow RFC 1423
	 * and is not supported by OpenSSL. In the case of OpenSSL 0.9.7
	 * it is possible to disable padding and do it by yourself
	 * For OpenSSL 0.9.6 you have interop problems
	 */
#ifndef XMLSEC_OPENSSL096	
	if(evpCipher->cipherCtx.cipher != NULL) {
	    b = evpCipher->cipherCtx.cipher->block_size;
	} else {
	    b = 0;
	}
	EVP_CIPHER_CTX_set_padding(&(evpCipher->cipherCtx), 0);    
#endif /* XMLSEC_OPENSSL096 */	
	ret = EVP_DecryptFinal(&(evpCipher->cipherCtx), 
		cipher->bufOut, &res);    		 		 
#ifndef XMLSEC_OPENSSL096
	if(ret == 1) {
	    res = (b > 0) ? b - cipher->bufOut[b - 1] : 0;
	    if(res < 0) {
		xmlSecError(XMLSEC_ERRORS_HERE,
			    XMLSEC_ERRORS_R_INVALID_DATA,
			    "padding is greater than buffer");
		return(-1);	
	    }
	}
#endif /* XMLSEC_OPENSSL096 */			
    }
    if(ret != 1) {
        xmlSecError(XMLSEC_ERRORS_HERE,
		    XMLSEC_ERRORS_R_CRYPTO_FAILED,
		    (cipher->encode) ? "EVP_EncryptFinal - %d" : "EVP_DecryptFinal - %d", ret);
	return(-1);	
    }
    return(res);    
}

/**
 * Misc EVP functions
 */
xmlSecKeyPtr	
xmlSecOpenSSLEvpParseKey(EVP_PKEY *pKey) {
    xmlSecKeyPtr key = NULL;
    xmlSecKeyValuePtr keyValue = NULL;
    int ret;
    
    xmlSecAssert2(pKey != NULL, NULL);

    switch(pKey->type) {	
#ifndef XMLSEC_NO_RSA    
    case EVP_PKEY_RSA:
	keyValue = xmlSecKeyValueCreate(xmlSecRsaKeyValue);
	if(keyValue == NULL) {
	    xmlSecError(XMLSEC_ERRORS_HERE,
			XMLSEC_ERRORS_R_XMLSEC_FAILED,
			"xmlSecKeyValueCreate");
	    return(NULL);	    
	}
	
	ret = xmlSecKeyValueSet(keyValue, pKey->pkey.rsa, sizeof(RSA));
	if(ret < 0) {	
	    xmlSecError(XMLSEC_ERRORS_HERE,
			XMLSEC_ERRORS_R_XMLSEC_FAILED,
			"xmlSecRsaKeyGenerate");
	    xmlSecKeyValueDestroy(keyValue);
	    return(NULL);	    
	}
	break;
#endif /* XMLSEC_NO_RSA */	
#ifndef XMLSEC_NO_DSA	
    case EVP_PKEY_DSA:
	keyValue = xmlSecKeyValueCreate(xmlSecDsaKeyValue);
	if(keyValue == NULL) {
	    xmlSecError(XMLSEC_ERRORS_HERE,
			XMLSEC_ERRORS_R_XMLSEC_FAILED,
			"xmlSecKeyValueCreate");
	    return(NULL);	    
	}
	
	ret = xmlSecKeyValueSet(keyValue, pKey->pkey.dsa, sizeof(DSA));
	if(ret < 0) {	
	    xmlSecError(XMLSEC_ERRORS_HERE,
			XMLSEC_ERRORS_R_XMLSEC_FAILED,
			"xmlSecDsaKeySet");
	    xmlSecKeyValueDestroy(keyValue);
	    return(NULL);	    
	}
	break;
#endif /* XMLSEC_NO_DSA */	
    default:	
	xmlSecError(XMLSEC_ERRORS_HERE,
		    XMLSEC_ERRORS_R_INVALID_KEY,
		    "key type %d not supported", pKey->type);
	return(NULL);
    }

    key = xmlSecKeyCreate(keyValue, NULL);
    if(key == NULL) {
        xmlSecError(XMLSEC_ERRORS_HERE,
		    XMLSEC_ERRORS_R_XMLSEC_FAILED,
		    "xmlSecKeyValueCreate");
	xmlSecKeyValueDestroy(keyValue);
	return(NULL);	    
    }
    xmlSecKeyValueDestroy(keyValue);
    
    return(key);
}
