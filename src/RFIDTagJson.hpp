#pragma once

#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>

// 保存するJSONファイル名
#define JSON_FILE "/tags.json"

class RFIDTagJson {
public:
    RFIDTagJson();
    bool isTagIdExists(const char* tagID);
    String getNameFromTagId(const char* tagID);
    bool addTagFromJson(const char* name, const char* tagID);
    bool deleteTagFromJson(const char* tagID);
    String[] listTag();
};

// ----------------------------------------------------------------------------------------------------------------------------------
// ==================================================================================================================================
// 処理の実装部分
// ==================================================================================================================================
// ----------------------------------------------------------------------------------------------------------------------------------

RFIDTagJson::RFIDTagJson() {
    if (!SPIFFS.begin()) {
        // Serial.println("SPIFFS Mount Failed");
        return;
    }
}

// タグIDが存在するか確認する関数
bool RFIDTagJson::isTagIdExists(const char* tagID) {
    // jsonファイルが存在しない場合、タグIDは存在しないと判断します。
    if (!SPIFFS.exists(JSON_FILE)) {
        return false;
    }

    // jsonファイルを開きます。
    File file = SPIFFS.open(JSON_FILE, FILE_READ);
    // ファイル読み取り失敗時
    if (!file) {
        return false;
    }

    // ファイルの内容をJSONオブジェクトにデシリアライズします。
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        // Serial.println("Failed to deserialize JSON");
        file.close();
        return false;
    }
    file.close();

    // JSONオブジェクト内を検索し、タグIDが存在するかどうかを確認します。
    for (JsonPair kv : doc.as<JsonObject>()) {
        if (strcmp(kv.value().as<const char*>(), tagID) == 0) {
            // 指定されたタグIDが見つかった場合
            return true;
        }
    }

    // 指定されたタグIDが見つからなかった場合
    return false;
}

// タグIDから名前を取得する関数
String RFIDTagJson::getNameFromTagId(const char* tagID) {
    // jsonファイルが存在しない場合、名前は見つかりません。
    if (!SPIFFS.exists(JSON_FILE)) {
        return "";
    }

    // jsonファイルを開きます。
    File file = SPIFFS.open(JSON_FILE, FILE_READ);
    // ファイル読み取り失敗時
    if (!file) {
        Serial.println("Failed to open file for reading");
        return "";
    }

    // ファイルの内容をJSONオブジェクトにデシリアライズします。
    StaticJsonDocument<1024> doc;
    // ファイル変換失敗時
    if (deserializeJson(doc, file) == 0) {
        file.close();
        return "";
    }
    file.close();

    // JSONオブジェクト内を検索し、タグIDに対応する名前を見つけます。
    for (JsonPair kv : doc.as<JsonObject>()) {
        if (strcmp(kv.value().as<const char*>(), tagID) == 0) {
            // 指定されたタグIDに対応する名前が見つかった場合、名前を返します。
            return String(kv.key().c_str());
        }
    }

    // 指定されたタグIDに対応する名前が見つからなかった場合、空文字列を返します。
    return "";
}

// タグ情報を追加する関数
bool RFIDTagJson::addTagFromJson(const char* name, const char* tagID) {
    // Jsonファイルが存在しない場合は何もしない
    if (!SPIFFS.exists(JSON_FILE)) {
        return false;
    }

    File file = SPIFFS.open(JSON_FILE, FILE_READ);
    // ファイル読み取り失敗時
    if (!file) {
        return false;
    }

    StaticJsonDocument<1024> doc;  // JSONドキュメントを作成します。
    DeserializationError error = deserializeJson(doc, file);
    // ファイル変換失敗時
    if (error) {
        file.close();
        return false;
    }
    file.close();  // ファイル読み込み完了後は閉じる

    // jsonファイルが存在する場合、その内容を読み込みます。
    if (SPIFFS.exists(JSON_FILE)) {
        File file = SPIFFS.open(JSON_FILE, FILE_READ);
        // ファイル読み取り失敗時
        if (!file) {
            return;
        }
        // ファイル変換失敗時
        if (deserializeJson(doc, file) == 0) {
        }
        file.close();
    }

    // 新しいタグ情報を追加します。
    doc[name] = tagID;

    // 変更をファイルに書き戻す
    File file = SPIFFS.open(JSON_FILE, FILE_WRITE);
    // ファイル書き込み失敗時
    if (!file) {
        return false;
    }

    // ファイル変換失敗時
    if (serializeJson(doc, file) == 0) {
        file.close();
        return false;
    }
    file.close();
    return true;
}
