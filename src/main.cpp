/*
作成日 : 2024/03/13
作成者 : 黒田 直樹

忘れ物を検知するバッグのコントロール側プログラム
デバイス : M5Dial
外部ペリフェラル : NeoPixel(WS2812B)

・機能
    忘れ物の登録、削除、確認を行う
    LEDの点灯、消灯を行う
    ブザーの鳴動を行う
    各機能のON/OFFを行う

*/

#include "main.hpp"

#include <M5Dial.h>
#include <M5Unified.h>

#include "RFIDTagJson.hpp"
#include "RFIDUart.hpp"
#include "pathImageFile.h"

#define HEIGHT_INTERVAL 40

RFIDTagJson tagJson;
RFIDUart rfidUart;

// 登録するもののカテゴリ
String category[] = {"サイフ", "カギ",         "スマホ", "パソコン",
                     "ノート", "カードケース", "その他"};
int categoryLength = sizeof(category) / sizeof(category[0]);
int categoryIndex = 0;

// ダイヤルポジション変数
long oldPosition = -999;
long newPosition = 0;

// かばんの中にものが存在するかどうか
bool isExistTag = false;
// LEDのON/OFFフラグ
bool isLedOn = true;
// ブザーのON/OFFフラグ
bool isBuzzerOn = true;

// メニュー画面の画像配置
String existTagImage[3] = {PATH_MENU_GREEN_BELL, PATH_MENU_GREEN_SETTING,
                           PATH_MENU_GREEN_ADD};
int existTagImageLength = sizeof(existTagImage) / sizeof(existTagImage[0]);
int existTagImageIndex = 0;
String notExistTagImage[3] = {PATH_MENU_RED_BELL, PATH_MENU_RED_SETTING,
                              PATH_MENU_RED_ADD};
int notExistTagImageLength =
    sizeof(notExistTagImage) / sizeof(notExistTagImage[0]);
int notExistTagImageIndex = 0;

// 設定画面の画像配置
String offOffImage[3] = {PATH_SETTING_TOP_OFF_OFF, PATH_SETTING_RIGHT_OFF_OFF,
                         PATH_SETTING_LEFT_OFF_OFF};
String onOffImage[3] = {PATH_SETTING_TOP_ON_OFF, PATH_SETTING_RIGHT_ON_OFF,
                        PATH_SETTING_LEFT_ON_OFF};
String offOnImage[3] = {PATH_SETTING_TOP_OFF_ON, PATH_SETTING_RIGHT_OFF_ON,
                        PATH_SETTING_LEFT_OFF_ON};
String onOnImage[3] = {PATH_SETTING_TOP_ON_ON, PATH_SETTING_RIGHT_ON_ON,
                       PATH_SETTING_LEFT_ON_ON};
int settingImageLength = sizeof(offOffImage) / sizeof(offOffImage[0]);
int settingImageIndex = 0;

// メニュー画面の選択肢
enum Loops
{
    MENU,
    SETTING,
    ADDTAG,
    TAGLIST
};

// 現在の選択肢
Loops currentLoops = MENU;

// 各画面のループ関数
void loop_menu(String tagImage[], int tagImageLength, int tagImageIndex);
void loop_setting();
void loop_addTag();
void loop_tagList();

// 各関数
void changeOnOffImage(int index);
void showList(int index);
int changeImageIndex(int index, int length, int direction);

void setup() {
    M5_BEGIN();
    auto cfg = M5.config();
    M5Dial.begin(cfg, true, false);

    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.setTextFont(&fonts::lgfxJapanGothic_36);
    M5Dial.Display.setTextSize(1);

    SPIFFS.begin();

    M5_UPDATE();
}

void loop() {
    M5Dial.update();
    // M5_UPDATE();

    // 状態によりループ関数を切り替える
    switch (currentLoops) {
        case MENU:
            if (isExistTag) {
                loop_menu(existTagImage, existTagImageLength,
                          existTagImageIndex);
            } else {
                loop_menu(notExistTagImage, notExistTagImageLength,
                          notExistTagImageIndex);
            }
            break;
        case SETTING:
            loop_setting();
            break;
        case ADDTAG:
            loop_addTag();
            break;
        case TAGLIST:
            loop_tagList();
            break;
    }

    // // ダイヤルがひねられたときの処理
    // if (newPosition != oldPosition) {
    //     oldPosition = newPosition;
    //     if (newPosition < 0) {
    //         newPosition *= -1;
    //     }
    //     uint8_t num = newPosition % 2;
    //     M5Dial.Speaker.tone(8000, 20);
    //     // これを入れると画面が切り替わるたびに黒飛びになる
    //     // M5Dial.Display.clear();
    //     if (num == 0) {
    //         M5.Lcd.drawJpgFile(SPIFFS, "/Menu_red.jpg", 4, 4);
    //     } else if (num == 1) {
    //         M5.Lcd.drawJpgFile(SPIFFS, "/Menu_green.jpg", 4, 4);
    //     }
    // }

    // 遅延を入れないとダイヤルの挙動がおかしくなる
    delay(1);
}

// メニュー画面のループ関数
void loop_menu(String tagImage[], int tagImageLength, int tagImageIndex) {
    newPosition = M5Dial.Encoder.read();

    // ダイヤルがひねられたときの処理
    if (newPosition != oldPosition) {
        M5Dial.Speaker.tone(8000, 20);
        tagImageIndex = changeImageIndex(tagImageIndex, tagImageLength,
                                         newPosition - oldPosition);
        oldPosition = newPosition;
        // 選択肢をもとに画像の切り替え
        M5.Lcd.drawJpgFile(SPIFFS, tagImage[tagImageIndex], 0, 0);
        existTagImageIndex = tagImageIndex;
        notExistTagImageIndex = tagImageIndex;
    }

    // ボタンが押されたときの処理
    if (M5Dial.BtnA.wasPressed()) {
        M5Dial.Speaker.tone(8000, 20);
        switch (tagImageIndex) {
            case 0:
                oldPosition = -999;
                currentLoops = TAGLIST;
                break;
            case 1:
                oldPosition = -999;
                currentLoops = SETTING;
                break;
            case 2:
                oldPosition = -999;
                currentLoops = ADDTAG;
                break;
        }
    }
}

// 設定画面のループ関数
void loop_setting() {
    newPosition = M5Dial.Encoder.read();

    // ダイヤルがひねられたときの処理
    if (newPosition != oldPosition) {
        M5Dial.Speaker.tone(8000, 20);
        settingImageIndex = changeImageIndex(
            settingImageIndex, settingImageLength, newPosition - oldPosition);
        oldPosition = newPosition;
        // 選択肢とブザー、LEDのON/OFFもとに画像の切り替え
        changeOnOffImage(settingImageIndex);
    }

    // ボタンが押されたときの処理
    if (M5Dial.BtnA.wasPressed()) {
        switch (settingImageIndex) {
            case 0:
                oldPosition = -999;
                currentLoops = MENU;
                break;
            case 1:
                isBuzzerOn = !isBuzzerOn;
                changeOnOffImage(settingImageIndex);
                break;
            case 2:
                isLedOn = !isLedOn;
                changeOnOffImage(settingImageIndex);
                break;
        }
    }
}

// タグ登録画面のループ関数
void loop_addTag() {
    newPosition = M5Dial.Encoder.read();

    // ダイヤルがひねられたときの処理
    if (newPosition != oldPosition) {
        M5Dial.Speaker.tone(8000, 20);
        categoryIndex = changeImageIndex(categoryIndex, categoryLength,
                                         newPosition - oldPosition);
        M5Dial.Display.fillScreen(0x4208);
        showList(categoryIndex);
        oldPosition = newPosition;
    }

    // ボタンが押されたときの処理
    if (M5Dial.BtnA.wasPressed()) {
        int count = 0;
        while (true) {
            count++;
            M5Dial.Display.fillScreen(0x4208);
            if (count % 3 == 0) {
                M5Dial.Display.drawString("タグをかざしてください.",
                                          M5Dial.Display.width() / 2,
                                          M5Dial.Display.height() / 2);
            } else if (count % 3 == 1) {
                M5Dial.Display.drawString("タグをかざしてください..",
                                          M5Dial.Display.width() / 2,
                                          M5Dial.Display.height() / 2);
            } else if (count % 3 == 2) {
                M5Dial.Display.drawString("タグをかざしてください...",
                                          M5Dial.Display.width() / 2,
                                          M5Dial.Display.height() / 2);
            }
            String tagId = rfidUart.getExistTagId();
            // タグIDが取得できた場合
            if (tagId != "") {
                // すでに登録されているタグIDの場合
                if (tagJson.isTagIdExists(tagId.c_str())) {
                    M5Dial.Display.fillScreen(0x4208);
                    M5Dial.Display.drawString("そのタグは",
                                              M5Dial.Display.width() / 2,
                                              M5Dial.Display.height() / 2 - 20);
                    M5Dial.Display.drawString("すでに登録されています.",
                                              M5Dial.Display.width() / 2,
                                              M5Dial.Display.height() / 2) +
                        20;
                    delay(2000);
                    continue;
                }
                // 未登録タグIDの場合
                else {
                    // タグIDを登録
                    if (tagJson.addTagFromJson(category[categoryIndex].c_str(),
                                               tagId.c_str())) {
                        M5Dial.Display.fillScreen(0x4208);
                        M5Dial.Display.drawString(
                            category[categoryIndex], M5Dial.Display.width() / 2,
                            M5Dial.Display.height() / 2 - 20);
                        M5Dial.Display.drawString("を登録しました。",
                                                  M5Dial.Display.width() / 2,
                                                  M5Dial.Display.height() / 2) +
                            20;
                        delay(2000);
                        break;
                    }
                    // タグIDの登録に失敗した場合
                    else {
                        M5Dial.Display.fillScreen(0x4208);
                        M5Dial.Display.drawString("登録に失敗しました.",
                                                  M5Dial.Display.width() / 2,
                                                  M5Dial.Display.height() / 2);
                        delay(2000);
                        break;
                    }
                }
            }
            delay(500);
        }
        currentLoops = MENU;
    }
}

// 設定画面の画像を切り替える関数
void changeOnOffImage(int index) {
    if (isLedOn) {
        if (isBuzzerOn) {
            M5.Lcd.drawJpgFile(SPIFFS, onOnImage[index], 0, 0);
        } else {
            M5.Lcd.drawJpgFile(SPIFFS, onOffImage[index], 0, 0);
        }
    } else {
        if (isBuzzerOn) {
            M5.Lcd.drawJpgFile(SPIFFS, offOnImage[index], 0, 0);
        } else {
            M5.Lcd.drawJpgFile(SPIFFS, offOffImage[index], 0, 0);
        }
    }
}

// タグリスト画面のループ関数
void loop_tagList() {
    newPosition = M5Dial.Encoder.read();

    // ダイヤルがひねられたときの処理
    if (newPosition != oldPosition) {
        M5Dial.Speaker.tone(8000, 20);
        categoryIndex = changeImageIndex(categoryIndex, categoryLength,
                                         newPosition - oldPosition);
        M5Dial.Display.fillScreen(0x4208);
        showList(categoryIndex);
        oldPosition = newPosition;
    }

    // ボタンが押されたときの処理
    if (M5Dial.BtnA.wasPressed()) {
        oldPosition = -999;
        currentLoops = MENU;
    }
}

// リストを表示する関数
void showList(int index) {
    for (int i = 0; i < categoryLength; i++) {
        int x = M5Dial.Display.width() / 2;
        int y = ((M5Dial.Display.height() / 2) + HEIGHT_INTERVAL * i) -
                index * HEIGHT_INTERVAL;
        if (i == index) {
            M5Dial.Display.setTextColor(RED);
        } else {
            M5Dial.Display.setTextColor(GREEN);
        }
        M5Dial.Display.drawString(category[i], x, y);
    }
}

// 画像のインデックスを変更する関数
int changeImageIndex(int index, int length, int direction) {
    // ダイヤルを時計回りに回したとき
    if (direction > 0) {
        index++;
        if (index >= length) {
            index = 0;
        }
    }
    // ダイヤルを反時計回りに回したとき
    else if (direction < 0) {
        index--;
        if (index < 0) {
            index = length - 1;
        }
    }
    return index;
}