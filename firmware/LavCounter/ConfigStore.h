#pragma once
#include <ArduinoJson.h>
#include <Preferences.h>
#include "Detection.h"

inline void configJson(JsonObject obj,const lav::Config& c){
 obj["revision"]=c.revision;
 for(size_t i=0;i<lav::FIELD_COUNT;i++){
  const auto& field=lav::FIELDS[i];unsigned int decimals=field.step>=1?0:field.step>=.1?1:3;
  // Stable decimal JSON keeps HTML number inputs aligned with their step after
  // a save/reload, rather than exposing float representation artifacts.
  obj[field.key]=serialized(String(c.v[i],decimals));
 }
}
inline bool parseConfig(JsonVariantConst input,lav::Config& c,JsonObject errors){
 if(!input.is<JsonObjectConst>()){errors["form"]="Envie todos os valores do formulário.";return false;}
 bool ok=true;
 for(size_t i=0;i<lav::FIELD_COUNT;i++){
  auto v=input[lav::FIELDS[i].key];
  if(!v.is<double>()||v.is<bool>()){errors[lav::FIELDS[i].key]="Informe um número válido.";ok=false;}
  else {
   double value=v.as<double>(),step=lav::FIELDS[i].step;
   double rounded=round(value/step)*step;
   if(!isfinite(value)||value<lav::FIELDS[i].min-step/100||value>lav::FIELDS[i].max+step/100||
      ((i==lav::MIN_PAUSES||i==lav::MIN_RESUMES||i==lav::SCORE)&&floor(value)!=value)){
    errors[lav::FIELDS[i].key]="Use um valor dentro dos limites e da precisão indicada.";ok=false;
   }
   else c.v[i]=rounded;
  }
 }
 if(!ok)return false;
 int bad=c.invalid();if(bad>=0){errors[lav::FIELDS[bad].key]="Valor fora dos limites ou incompatível com os demais campos.";return false;}
 return true;
}
class ConfigStore {
 Preferences prefs; bool ready=false;
public:
 lav::Config current; String error;
 bool begin(){
  ready=prefs.begin("lavcounter",false);if(!ready){error="Não foi possível abrir a configuração.";return false;}
  String saved=prefs.getString("config","");if(saved.isEmpty())return true;
  JsonDocument d,errors;
  if(deserializeJson(d,saved)||!parseConfig(d.as<JsonVariantConst>(),current,errors.to<JsonObject>())){current=lav::Config{};error="Configuração inválida; padrões carregados. Salve para corrigir.";return false;}
  current.revision=d["revision"]|1U;return true;
 }
 bool save(lav::Config next){
  if(!ready||next.invalid()>=0||current.revision==UINT32_MAX){error="Não foi possível salvar a configuração.";return false;}
  next.revision=current.revision+1;JsonDocument d;configJson(d.to<JsonObject>(),next);String value;serializeJson(d,value);
  if(prefs.putString("config",value)!=value.length()){error="Falha ao persistir; configuração anterior mantida.";return false;}
  current=next;error="";return true;
 }
 uint64_t reserveId(){
  if(!ready)return 0;uint64_t next=prefs.getULong64("nextId",1);
  if(next==0||next==UINT64_MAX||prefs.putULong64("nextId",next+1)!=sizeof(uint64_t)){error="Falha ao reservar identificador do evento.";return 0;}return next;
 }
 void describe(JsonDocument& d){
  configJson(d["values"].to<JsonObject>(),current);lav::Config defaults;configJson(d["defaults"].to<JsonObject>(),defaults);
  d["revision"]=current.revision;d["error"]=error;
  JsonArray fields=d["fields"].to<JsonArray>();
  for(const auto& f:lav::FIELDS){
   JsonObject o=fields.add<JsonObject>();o["key"]=f.key;o["label"]=f.label;o["unit"]=f.unit;o["help"]=f.help;
   o["min"]=serialized(String(f.min,3U));o["max"]=serialized(String(f.max,3U));o["step"]=serialized(String(f.step,3U));o["group"]=f.group;
  }
 }
};

