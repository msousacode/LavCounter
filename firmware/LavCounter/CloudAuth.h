#pragma once
#include "CloudConfig.h"
namespace lavcloud {
struct AuthToken {String value;uint64_t expiresAt=0;bool valid(uint64_t utc)const{return utc>=1700000000&&expiresAt>utc+30&&value.length()>0&&value.length()<=4096;}};
class AuthProvider {public:virtual ~AuthProvider()=default;virtual bool acquire(uint64_t utc,bool refresh,AuthToken& out)=0;};
class UnavailableAuth:public AuthProvider {public:bool acquire(uint64_t,bool,AuthToken& out)override{out=AuthToken{};return false;}};
enum class PremiumStatus {Unknown,Inactive,Active,Expired};
inline const char* premiumName(PremiumStatus s){return s==PremiumStatus::Active?"active":s==PremiumStatus::Inactive?"inactive":s==PremiumStatus::Expired?"expired":"unknown";}
struct PremiumGrant {
 PremiumStatus status=PremiumStatus::Unknown;String binding;uint64_t expiresAt=0;
 bool allows(const String& expected,uint64_t utc)const{return utc>=1700000000&&status==PremiumStatus::Active&&binding==expected.c_str()&&expiresAt>utc;}
 PremiumStatus effective(uint64_t utc)const{return status==PremiumStatus::Active&&expiresAt<=utc?PremiumStatus::Expired:status;}
};
class PremiumProvider {public:virtual ~PremiumProvider()=default;virtual bool check(const CloudConfig& config,const AuthToken& token,const String& binding,uint64_t utc,PremiumGrant& out)=0;};
class UnavailablePremium:public PremiumProvider {public:bool check(const CloudConfig&,const AuthToken&,const String&,uint64_t,PremiumGrant& out)override{out=PremiumGrant{};return false;}};
}
