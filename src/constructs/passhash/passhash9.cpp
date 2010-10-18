/*
* Passhash9 Password Hashing
* (C) 2010 Jack Lloyd
*
* Distributed under the terms of the Botan license
*/

#include <botan/passhash9.h>
#include <botan/loadstor.h>
#include <botan/libstate.h>
#include <botan/pbkdf2.h>
#include <botan/base64.h>
#include <botan/pipe.h>

namespace Botan {

namespace {

const std::string MAGIC_PREFIX = "$9$";

const size_t WORKFACTOR_BYTES = 2;
const size_t ALGID_BYTES = 1;
const size_t SALT_BYTES = 12; // 96 bits of salt
const size_t PASSHASH9_PBKDF_OUTPUT_LEN = 24; // 192 bits output

const byte PASSHASH9_DEFAULT_ALGO = 0; // HMAC(SHA-1)

const size_t WORK_FACTOR_SCALE = 10000;

MessageAuthenticationCode* get_pbkdf_prf(byte alg_id)
   {
   Algorithm_Factory& af = global_state().algorithm_factory();

   try
      {
      if(alg_id == 0)
         return af.make_mac("HMAC(SHA-1)");
      else if(alg_id == 1)
         return af.make_mac("HMAC(SHA-256)");
      else if(alg_id == 2)
         return af.make_mac("CMAC(Blowfish)");
      }
   catch(Algorithm_Not_Found) {}

   return 0;
   }

}

std::string generate_passhash9(const std::string& pass,
                               RandomNumberGenerator& rng,
                               u16bit work_factor)
   {
   return generate_passhash9(pass, PASSHASH9_DEFAULT_ALGO, rng, work_factor);
   }

std::string generate_passhash9(const std::string& pass,
                               byte alg_id,
                               RandomNumberGenerator& rng,
                               u16bit work_factor)
   {
   MessageAuthenticationCode* prf = get_pbkdf_prf(alg_id);

   if(!prf)
      throw Invalid_Argument("Passhash9: Algorithm id " + to_string(alg_id) +
                             " is not defined");

   PKCS5_PBKDF2 kdf(prf); // takes ownership of pointer

   SecureVector<byte> salt(SALT_BYTES);
   rng.randomize(&salt[0], salt.size());

   const size_t kdf_iterations = WORK_FACTOR_SCALE * work_factor;

   SecureVector<byte> pbkdf2_output =
      kdf.derive_key(PASSHASH9_PBKDF_OUTPUT_LEN,
                     pass,
                     &salt[0], salt.size(),
                     kdf_iterations).bits_of();

   Pipe pipe(new Base64_Encoder);
   pipe.start_msg();
   pipe.write(alg_id);
   pipe.write(get_byte(0, work_factor));
   pipe.write(get_byte(1, work_factor));
   pipe.write(salt);
   pipe.write(pbkdf2_output);
   pipe.end_msg();

   return MAGIC_PREFIX + pipe.read_all_as_string();
   }

bool check_passhash9(const std::string& pass, const std::string& hash)
   {
   const size_t BINARY_LENGTH =
     ALGID_BYTES +
     WORKFACTOR_BYTES +
     PASSHASH9_PBKDF_OUTPUT_LEN +
     SALT_BYTES;

   const size_t BASE64_LENGTH =
      MAGIC_PREFIX.size() + (BINARY_LENGTH * 8) / 6;

   if(hash.size() != BASE64_LENGTH)
      return false;

   for(size_t i = 0; i != MAGIC_PREFIX.size(); ++i)
      if(hash[i] != MAGIC_PREFIX[i])
         return false;

   Pipe pipe(new Base64_Decoder);
   pipe.start_msg();
   pipe.write(hash.c_str() + MAGIC_PREFIX.size());
   pipe.end_msg();

   SecureVector<byte> bin = pipe.read_all();

   if(bin.size() != BINARY_LENGTH)
      return false;

   byte alg_id = bin[0];

   const size_t kdf_iterations =
      WORK_FACTOR_SCALE * load_be<u16bit>(&bin[ALGID_BYTES], 0);

   if(kdf_iterations == 0)
      return false;

   MessageAuthenticationCode* pbkdf_prf = get_pbkdf_prf(alg_id);

   if(pbkdf_prf == 0)
      return false; // unknown algorithm, reject

   PKCS5_PBKDF2 kdf(pbkdf_prf); // takes ownership of pointer

   SecureVector<byte> cmp = kdf.derive_key(
      PASSHASH9_PBKDF_OUTPUT_LEN,
      pass,
      &bin[ALGID_BYTES + WORKFACTOR_BYTES], SALT_BYTES,
      kdf_iterations).bits_of();

   return same_mem(&cmp[0],
                   &bin[ALGID_BYTES + WORKFACTOR_BYTES + SALT_BYTES],
                   PASSHASH9_PBKDF_OUTPUT_LEN);
   }

}
