#include "basic_types.h"
#include <sstream>
#include <iomanip>

// ZeroKnowledgeProof 序列化实现
std::string ZeroKnowledgeProof::serialize() const {
    std::ostringstream oss;
    
    // 序列化证明类型
    oss << static_cast<int>(type) << "|";
    
    // 序列化承诺
    if (commitment.valid()) {
        oss << commitment.get_value().to_string() << "|" 
            << commitment.get_modulus().to_string() << "|";
    } else {
        oss << "0|0|";
    }
    
    // 序列化挑战和响应
    oss << challenge.to_string() << "|" 
        << response.to_string() << "|" 
        << randomness.to_string() << "|";
    
    // 序列化辅助数据
    oss << auxiliary_data.size() << "|";
    for (const auto& aux : auxiliary_data) {
        if (aux.valid()) {
            oss << aux.get_value().to_string() << "|" 
                << aux.get_modulus().to_string() << "|";
        } else {
            oss << "0|0|";
        }
    }
    
    // 序列化有效性标志
    oss << (is_valid ? "1" : "0");
    
    return oss.str();
}

ZeroKnowledgeProof ZeroKnowledgeProof::deserialize(const std::string& data) {
    ZeroKnowledgeProof proof(ProofType::MEMBERSHIP);
    std::istringstream iss(data);
    std::string token;
    
    // 解析证明类型
    if (std::getline(iss, token, '|')) {
        int type_int = std::stoi(token);
        proof.type = static_cast<ProofType>(type_int);
    }
    
    // 解析承诺
    std::string comm_value, comm_mod;
    if (std::getline(iss, comm_value, '|') && std::getline(iss, comm_mod, '|')) {
        if (comm_value != "0" && comm_mod != "0") {
            BigInt value(comm_value);
            BigInt modulus(comm_mod);
            proof.commitment = GroupElement(value, modulus);
        }
    }
    
    // 解析挑战、响应和随机数
    std::string chall, resp, rand;
    if (std::getline(iss, chall, '|') && 
        std::getline(iss, resp, '|') && 
        std::getline(iss, rand, '|')) {
        proof.challenge = BigInt(chall);
        proof.response = BigInt(resp);
        proof.randomness = BigInt(rand);
    }
    
    // 解析辅助数据
    std::string aux_size_str;
    if (std::getline(iss, aux_size_str, '|')) {
        size_t aux_size = std::stoull(aux_size_str);
        for (size_t i = 0; i < aux_size; ++i) {
            std::string aux_value, aux_mod;
            if (std::getline(iss, aux_value, '|') && std::getline(iss, aux_mod, '|')) {
                if (aux_value != "0" && aux_mod != "0") {
                    BigInt value(aux_value);
                    BigInt modulus(aux_mod);
                    proof.auxiliary_data.push_back(GroupElement(value, modulus));
                }
            }
        }
    }
    
    // 解析有效性标志
    std::string valid_str;
    if (std::getline(iss, valid_str)) {
        proof.is_valid = (valid_str == "1");
    }
    
    return proof;
}
