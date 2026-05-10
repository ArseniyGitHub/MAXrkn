#include <openssl/sha.h>
#include <string>
#include <sstream>
#include <iomanip>

const std::string calculate_sha256(const std::string& data) {
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), hash);
	std::stringstream stream;
	for (unsigned char i = 0; i < SHA256_DIGEST_LENGTH; i++)
		stream << std::hex << std::setw(2) << std::setfill('0') << ((int)hash[i]);
	return stream.str();
}
