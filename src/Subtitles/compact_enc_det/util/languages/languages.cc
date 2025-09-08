// Copyright 2016 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////////

#include "util/languages/languages.h"

#include "util/basictypes.h"
#include "util/string_util.h"


Language default_language() {return ENGLISH;}


// Language names and codes

struct LanguageInfo {
  const char * language_name_;
  const char * language_code_639_1_;   // the ISO-639-1 code for the language
  const char * language_code_639_2_;   // the ISO-639-2 code for the language
  const char * language_code_other_;   // some nonstandard code for the language
};

static const LanguageInfo kLanguageInfoTable[] = {
  { "ENGLISH",             "en", "eng", NULL},
  { "DANISH",              "da", "dan", NULL},
  { "DUTCH",               "nl", "dut", NULL},
  { "FINNISH",             "fi", "fin", NULL},
  { "FRENCH",              "fr", "fre", NULL},
  { "GERMAN",              "de", "ger", NULL},
  { "HEBREW",              "he", "heb", NULL},
  { "ITALIAN",             "it", "ita", NULL},
  { "Japanese",            "ja", "jpn", NULL},
  { "Korean",              "ko", "kor", NULL},
  { "NORWEGIAN",           "nb", "nor", NULL},
  { "POLISH",              "pl", "pol", NULL},
  { "PORTUGUESE",          "pt", "por", NULL},
  { "RUSSIAN",             "ru", "rus", NULL},
  { "SPANISH",             "es", "spa", NULL},
  { "SWEDISH",             "sv", "swe", NULL},
  { "Chinese",             "zh", "chi", "zh-CN"},
  { "CZECH",               "cs", "cze", NULL},
  { "GREEK",               "el", "gre", NULL},
  { "ICELANDIC",           "is", "ice", NULL},
  { "LATVIAN",             "lv", "lav", NULL},
  { "LITHUANIAN",          "lt", "lit", NULL},
  { "ROMANIAN",            "ro", "rum", NULL},
  { "HUNGARIAN",           "hu", "hun", NULL},
  { "ESTONIAN",            "et", "est", NULL},
  // TODO: Although Teragram has two output names "TG_UNKNOWN_LANGUAGE"
  // and "Unknown", they are essentially the same. Need to unify them.
  // "un" and "ut" are invented by us, not from ISO-639.
  //
  { "TG_UNKNOWN_LANGUAGE", NULL, NULL, "ut"},
  { "Unknown",             NULL, NULL, "un"},
  { "BULGARIAN",           "bg", "bul", NULL},
  { "CROATIAN",            "hr", "scr", NULL},
  { "SERBIAN",             "sr", "scc", NULL},
  { "IRISH",               "ga", "gle", NULL},
  { "GALICIAN",            "gl", "glg", NULL},
  // Impossible to tell Tagalog from Filipino at the moment.
  // Use ISO 639-2 code for Filipino here.
  { "TAGALOG",             NULL, "fil", NULL},
  { "TURKISH",             "tr", "tur", NULL},
  { "UKRAINIAN",           "uk", "ukr", NULL},
  { "HINDI",               "hi", "hin", NULL},
  { "MACEDONIAN",          "mk", "mac", NULL},
  { "BENGALI",             "bn", "ben", NULL},
  { "INDONESIAN",          "id", "ind", NULL},
  { "LATIN",               "la", "lat", NULL},
  { "MALAY",               "ms", "may", NULL},
  { "MALAYALAM",           "ml", "mal", NULL},
  { "WELSH",               "cy", "wel", NULL},
  { "NEPALI",              "ne", "nep", NULL},
  { "TELUGU",              "te", "tel", NULL},
  { "ALBANIAN",            "sq", "alb", NULL},
  { "TAMIL",               "ta", "tam", NULL},
  { "BELARUSIAN",          "be", "bel", NULL},
  { "JAVANESE",            "jw", "jav", NULL},
  { "OCCITAN",             "oc", "oci", NULL},
  { "URDU",                "ur", "urd", NULL},
  { "BIHARI",              "bh", "bih", NULL},
  { "GUJARATI",            "gu", "guj", NULL},
  { "THAI",                "th", "tha", NULL},
  { "ARABIC",              "ar", "ara", NULL},
  { "CATALAN",             "ca", "cat", NULL},
  { "ESPERANTO",           "eo", "epo", NULL},
  { "BASQUE",              "eu", "baq", NULL},
  { "INTERLINGUA",         "ia", "ina", NULL},
  { "KANNADA",             "kn", "kan", NULL},
  { "PUNJABI",             "pa", "pan", NULL},
  { "SCOTS_GAELIC",        "gd", "gla", NULL},
  { "SWAHILI",             "sw", "swa", NULL},
  { "SLOVENIAN",           "sl", "slv", NULL},
  { "MARATHI",             "mr", "mar", NULL},
  { "MALTESE",             "mt", "mlt", NULL},
  { "VIETNAMESE",          "vi", "vie", NULL},
  { "FRISIAN",             "fy", "fry", NULL},
  { "SLOVAK",              "sk", "slo", NULL},
  { "ChineseT",
    NULL,  NULL,  // We intentionally set these 2 fields to NULL to avoid
                  // confusion between CHINESE_T and CHINESE.
    "zh-TW"},
  { "FAROESE",             "fo", "fao", NULL},
  { "SUNDANESE",           "su", "sun", NULL},
  { "UZBEK",               "uz", "uzb", NULL},
  { "AMHARIC",             "am", "amh", NULL},
  { "AZERBAIJANI",         "az", "aze", NULL},
  { "GEORGIAN",            "ka", "geo", NULL},
  { "TIGRINYA",            "ti", "tir", NULL},
  { "PERSIAN",             "fa", "per", NULL},
  { "BOSNIAN",             "bs", "bos", NULL},
  { "SINHALESE",           "si", "sin", NULL},
  { "NORWEGIAN_N",         "nn", "nno", NULL},
  { "PORTUGUESE_P",        NULL, NULL, "pt-PT"},
  { "PORTUGUESE_B",        NULL, NULL, "pt-BR"},
  { "XHOSA",               "xh", "xho", NULL},
  { "ZULU",                "zu", "zul", NULL},
  { "GUARANI",             "gn", "grn", NULL},
  { "SESOTHO",             "st", "sot", NULL},
  { "TURKMEN",             "tk", "tuk", NULL},
  { "KYRGYZ",              "ky", "kir", NULL},
  { "BRETON",              "br", "bre", NULL},
  { "TWI",                 "tw", "twi", NULL},
  { "YIDDISH",             "yi", "yid", NULL},
  { "SERBO_CROATIAN",      "sh", NULL, NULL},
  { "SOMALI",              "so", "som", NULL},
  { "UIGHUR",              "ug", "uig", NULL},
  { "KURDISH",             "ku", "kur", NULL},
  { "MONGOLIAN",           "mn", "mon", NULL},
  { "ARMENIAN",            "hy", "arm", NULL},
  { "LAOTHIAN",            "lo", "lao", NULL},
  { "SINDHI",              "sd", "snd", NULL},
  { "RHAETO_ROMANCE",      "rm", "roh", NULL},
  { "AFRIKAANS",           "af", "afr", NULL},
  { "LUXEMBOURGISH",       "lb", "ltz", NULL},
  { "BURMESE",             "my", "bur", NULL},
  // KHMER is known as Cambodian for Google user interfaces.
  { "KHMER",               "km", "khm", NULL},
  { "TIBETAN",             "bo", "tib", NULL},
  { "DHIVEHI",             "dv", "div", NULL},
  { "CHEROKEE",            NULL, "chr", NULL},
  { "SYRIAC",              NULL, "syr", NULL},
  { "LIMBU",               NULL, NULL, "sit-NP"},
  { "ORIYA",               "or", "ori", NULL},
  { "ASSAMESE",            "as", "asm", NULL},
  { "CORSICAN",            "co", "cos", NULL},
  { "INTERLINGUE",         "ie", "ine", NULL},
  { "KAZAKH",              "kk", "kaz", NULL},
  { "LINGALA",             "ln", "lin", NULL},
  { "MOLDAVIAN",           "mo", "mol", NULL},
  { "PASHTO",              "ps", "pus", NULL},
  { "QUECHUA",             "qu", "que", NULL},
  { "SHONA",               "sn", "sna", NULL},
  { "TAJIK",               "tg", "tgk", NULL},
  { "TATAR",               "tt", "tat", NULL},
  { "TONGA",               "to", "tog", NULL},
  { "YORUBA",              "yo", "yor", NULL},
  { "CREOLES_AND_PIDGINS_ENGLISH_BASED", NULL, "cpe", NULL},
  { "CREOLES_AND_PIDGINS_FRENCH_BASED",  NULL, "cpf", NULL},
  { "CREOLES_AND_PIDGINS_PORTUGUESE_BASED", NULL, "cpp", NULL},
  { "CREOLES_AND_PIDGINS_OTHER", NULL, "crp", NULL},
  { "MAORI",               "mi", "mao", NULL},
  { "WOLOF",               "wo", "wol", NULL},
  { "ABKHAZIAN",           "ab", "abk", NULL},
  { "AFAR",                "aa", "aar", NULL},
  { "AYMARA",              "ay", "aym", NULL},
  { "BASHKIR",             "ba", "bak", NULL},
  { "BISLAMA",             "bi", "bis", NULL},
  { "DZONGKHA",            "dz", "dzo", NULL},
  { "FIJIAN",              "fj", "fij", NULL},
  { "GREENLANDIC",         "kl", "kal", NULL},
  { "HAUSA",               "ha", "hau", NULL},
  { "HAITIAN_CREOLE",       "ht", NULL, NULL},
  { "INUPIAK",             "ik", "ipk", NULL},
  { "INUKTITUT",           "iu", "iku", NULL},
  { "KASHMIRI",            "ks", "kas", NULL},
  { "KINYARWANDA",         "rw", "kin", NULL},
  { "MALAGASY",            "mg", "mlg", NULL},
  { "NAURU",               "na", "nau", NULL},
  { "OROMO",               "om", "orm", NULL},
  { "RUNDI",               "rn", "run", NULL},
  { "SAMOAN",              "sm", "smo", NULL},
  { "SANGO",               "sg", "sag", NULL},
  { "SANSKRIT",            "sa", "san", NULL},
  { "SISWANT",             "ss", "ssw", NULL},
  { "TSONGA",              "ts", "tso", NULL},
  { "TSWANA",              "tn", "tsn", NULL},
  { "VOLAPUK",             "vo", "vol", NULL},
  { "ZHUANG",              "za", "zha", NULL},
  { "KHASI",               NULL, "kha", NULL},
  { "SCOTS",               NULL, "sco", NULL},
  { "GANDA",               "lg", "lug", NULL},
  { "MANX",                "gv", "glv", NULL},
  { "MONTENEGRIN",         NULL, NULL, "sr-ME"},
  { "XX",                  NULL, NULL, "XX"},
};

COMPILE_ASSERT(arraysize(kLanguageInfoTable) == NUM_LANGUAGES + 1,
               kLanguageInfoTable_has_incorrect_length);


// LANGUAGE NAMES

const char* default_language_name() {
  return kLanguageInfoTable[ENGLISH].language_name_;
}

static const char* const kInvalidLanguageName = "invalid_language";

const char *invalid_language_name() {
  return kInvalidLanguageName;
}

const char* LanguageName(Language lang) {
  return IsValidLanguage(lang)
      ? kLanguageInfoTable[lang].language_name_
      : kInvalidLanguageName;
}



// LANGUAGE CODES


// The space before invalid_language_code is intentional. It is used
// to prevent it matching any two letter language code.
//
static const char* const kInvalidLanguageCode = " invalid_language_code";

const char *invalid_language_code() {
  return kInvalidLanguageCode;
}

const char * LanguageCode(Language lang) {
  if (! IsValidLanguage(lang))
    return kInvalidLanguageCode;
  const LanguageInfo& info = kLanguageInfoTable[lang];
  if (info.language_code_639_1_) {
    return info.language_code_639_1_;
  } else if (info.language_code_639_2_) {
    return info.language_code_639_2_;
  } else if (info.language_code_other_) {
    return info.language_code_other_;
  } else {
    return kInvalidLanguageCode;
  }
}

const char* default_language_code() {
  return kLanguageInfoTable[ENGLISH].language_code_639_1_;
}

const char* LanguageCodeISO639_1(Language lang) {
  if (! IsValidLanguage(lang))
    return kInvalidLanguageCode;
  if (const char* code = kLanguageInfoTable[lang].language_code_639_1_)
    return code;
  return kInvalidLanguageCode;
}

const char* LanguageCodeISO639_2(Language lang) {
  if (! IsValidLanguage(lang))
    return kInvalidLanguageCode;
  if (const char* code = kLanguageInfoTable[lang].language_code_639_2_)
    return code;
  return kInvalidLanguageCode;
}

const char* LanguageCodeWithDialects(Language lang) {
  if (lang == CHINESE)
    return "zh-CN";
  return LanguageCode(lang);
}



bool LanguageFromCode(const char* lang_code, Language *language) {
  *language = UNKNOWN_LANGUAGE;
  if ( lang_code == NULL ) return false;

  for ( int i = 0 ; i < kNumLanguages ; i++ ) {
    const LanguageInfo& info = kLanguageInfoTable[i];
    if ((info.language_code_639_1_ &&
         !base::strcasecmp(lang_code, info.language_code_639_1_)) ||
        (info.language_code_639_2_ &&
         !base::strcasecmp(lang_code, info.language_code_639_2_)) ||
        (info.language_code_other_ &&
         !base::strcasecmp(lang_code, info.language_code_other_))) {
      *language = static_cast<Language>(i);
      return true;
    }
  }

  // For convenience, this function can also parse the non-standard
  // five-letter language codes "zh-cn" and "zh-tw" which are used by
  // front-ends such as GWS to distinguish Simplified from Traditional
  // Chinese.
  if (!base::strcasecmp(lang_code, "zh-cn") ||
      !base::strcasecmp(lang_code, "zh_cn")) {
    *language = CHINESE;
    return true;
  }
  if (!base::strcasecmp(lang_code, "zh-tw") ||
      !base::strcasecmp(lang_code, "zh_tw")) {
    *language = CHINESE_T;
    return true;
  }
  if (!base::strcasecmp(lang_code, "sr-me") ||
      !base::strcasecmp(lang_code, "sr_me")) {
    *language = MONTENEGRIN;
    return true;
  }

  // Process language-code synonyms.
  if (!base::strcasecmp(lang_code, "he")) {
    *language = HEBREW;  // Use "iw".
    return true;
  }
  if (!base::strcasecmp(lang_code, "in")) {
    *language = INDONESIAN;  // Use "id".
    return true;
  }
  if (!base::strcasecmp(lang_code, "ji")) {
    *language = YIDDISH;  // Use "yi".
    return true;
  }

  // Process language-detection synonyms.
  // These distinct languages cannot be differentiated by our current
  // language-detection algorithms.
  if (!base::strcasecmp(lang_code, "fil")) {
    *language = TAGALOG;
    return true;
  }

  return false;
}
