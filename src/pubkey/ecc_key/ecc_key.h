/*
* ECDSA
* (C) 2007 Falko Strenzke, FlexSecure GmbH
*          Manuel Hartl, FlexSecure GmbH
* (C) 2008-2010 Jack Lloyd
*
* Distributed under the terms of the Botan license
*/

#ifndef BOTAN_ECC_PUBLIC_KEY_BASE_H__
#define BOTAN_ECC_PUBLIC_KEY_BASE_H__

#include <botan/ec_dompar.h>
#include <botan/pk_keys.h>
#include <botan/x509_key.h>
#include <botan/pkcs8.h>
#include <memory>

namespace Botan {

/**
* This class represents abstract EC Public Keys. When encoding a key
* via an encoder that can be accessed via the corresponding member
* functions, the key will decide upon its internally stored encoding
* information whether to encode itself with or without domain
* parameters, or using the domain parameter oid. Furthermore, a public
* key without domain parameters can be decoded. In that case, it
* cannot be used for verification until its domain parameters are set
* by calling the corresponding member function.
*/
class BOTAN_DLL EC_PublicKey : public virtual Public_Key
   {
   public:

      /**
      * Get the public point of this key.
      * @throw Invalid_State is thrown if the
      * domain parameters of this point are not set
      * @result the public point of this key
      */
      const PointGFp& public_point() const { return public_key; }

      AlgorithmIdentifier algorithm_identifier() const;

      MemoryVector<byte> x509_subject_public_key() const;

      /**
      * Get the domain parameters of this key.
      * @throw Invalid_State is thrown if the
      * domain parameters of this point are not set
      * @result the domain parameters of this key
      */
      const EC_Domain_Params& domain() const { return domain_params; }

      /**
      * Set the domain parameter encoding to be used when encoding this key.
      * @param enc the encoding to use
      */
      void set_parameter_encoding(EC_Domain_Params_Encoding enc);

      /**
      * Return the DER encoding of this keys domain in whatever format
      * is preset for this particular key
      */
      MemoryVector<byte> DER_domain() const
         { return domain().DER_encode(domain_format()); }

      /**
      * Get the domain parameter encoding to be used when encoding this key.
      * @result the encoding to use
      */
      EC_Domain_Params_Encoding domain_format() const
         { return domain_encoding; }

      /**
      * Get an x509_decoder that can be used to decode a stored key into
      * this key.
      * @result an x509_decoder for this key
      */
      X509_Decoder* x509_decoder();

      EC_PublicKey() : domain_encoding(EC_DOMPAR_ENC_EXPLICIT) {}

      EC_PublicKey(const EC_Domain_Params& dom_par,
                   const PointGFp& pub_point);

      virtual ~EC_PublicKey() {}
   protected:
      virtual void X509_load_hook();

      EC_Domain_Params domain_params;
      PointGFp public_key;
      EC_Domain_Params_Encoding domain_encoding;
   };

/**
* This abstract class represents general EC Private Keys
*/
class BOTAN_DLL EC_PrivateKey : public virtual EC_PublicKey,
                                public virtual Private_Key
   {
   public:
      EC_PrivateKey() {}

      EC_PrivateKey(const EC_Domain_Params& domain,
                    const BigInt& private_key);

      EC_PrivateKey(RandomNumberGenerator& rng,
                    const EC_Domain_Params& domain);

      virtual ~EC_PrivateKey() {}

      MemoryVector<byte> pkcs8_private_key() const;

      /**
      * Get an PKCS#8 encoder that can be used to encoded this key.
      * @result an PKCS#8 encoder for this key
      */
      PKCS8_Encoder* pkcs8_encoder() const;

      /**
      * Get an PKCS#8 decoder that can be used to decoded a stored key into
      * this key.
      * @result an PKCS#8 decoder for this key
      */
      PKCS8_Decoder* pkcs8_decoder(RandomNumberGenerator&);

      /**
      * Get the private key value of this key object.
      * @result the private key value of this key object
      */
      const BigInt& private_value() const;
   protected:
      virtual void PKCS8_load_hook(bool = false);

      BigInt private_key;
   };

}

#endif
