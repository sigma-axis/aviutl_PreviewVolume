# AviUtl プレビュー音量操作プラグイン

AviUtl でプレビュー再生する際の音量を調整するウィンドウを追加するプラグインです．ミュート切り替え機能付き．

![動作デモ](https://github.com/user-attachments/assets/d5f8da48-180a-4060-97ef-76855671fd36)

プレビュー再生中のメインウィンドウ左下のゲージを操作する別ウィンドウです．[アルティメットプラグイン](https://github.com/hebiiro/anti.aviutl.ultimate.plugin)の[ワークスペース化アドイン](https://github.com/hebiiro/anti.aviutl.ultimate.plugin/wiki/workspace)との併用を想定しています．

[ダウンロードはこちら．](https://github.com/sigma-axis/aviutl_PreviewVolume/releases)

## 動作要件

- AviUtl 1.10

  http://spring-fragrance.mints.ne.jp/aviutl
  - AviUtl 1.00 等の他バージョンでは動作しません．

- Visual C++ 再頒布可能パッケージ（\[2015/2017/2019/2022\] の x86 対応版が必要）

  https://learn.microsoft.com/ja-jp/cpp/windows/latest-supported-vc-redist

- **(推奨)** アルティメットプラグイン

  ワークスペース化でどこか小さな隙間に収められます．

  https://github.com/hebiiro/anti.aviutl.ultimate.plugin


## 導入方法

以下のフォルダのいずれかに `PreviewVolume.auf` と `PreviewVolume.ini` をコピーしてください．

1. `aviutl.exe` のあるフォルダ
1. (1) のフォルダにある `plugins` フォルダ
1. (2) のフォルダにある任意のフォルダ


## 使い方

AviUtlメインウィンドウの「設定」メニューから「プレビュー音量の設定」を選択してウィンドウを表示させてください．

アルティメットのワークスペース化アドインを導入している場合，ドッキング対象選択メニューの「セカンダリ」ウィンドウリストから「プレビュー音量」を選択してください．

- スライダーで音量を調節します．

- 左（または下）にあるボタンでミュート / ミュート解除ができます．

- 縦型と横型を[設定ファイルの編集](#設定ファイルについて)で選ぶことが来ます．ミュートボタンを非表示にもできます．

## 設定ファイルについて

`PreviewVolume.ini` をテキストエディタで編集することで好みのレイアウトを選ぶことができます．ファイル内のコメントにも説明があるのでそちらも併せて参照してください．

### `[Layout]`

- `vertical` で縦型と横型のどちらにするかを選ぶことができます．

- `mute_button` でミュートボタンの表示・非表示を選択できます．

初期値は以下の通り:

```ini
[Layout]
vertical=0
mute_button=1
```
- 横型の配置で，ミュートボタンを表示．


## 改版履歴

- **v1.00** (2024-07-25)

  - 初版．


## ライセンス・免責事項

このプログラムの利用・改変・再頒布等に関しては CC0 として扱うものとします．


#  Credits

##  aviutl_exedit_sdk

https://github.com/ePi5131/aviutl_exedit_sdk （利用したブランチは[こちら](https://github.com/sigma-axis/aviutl_exedit_sdk/tree/self-use)です．）

---

1条項BSD

Copyright (c) 2022
ePi All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
THIS SOFTWARE IS PROVIDED BY ePi “AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL ePi BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#  連絡・バグ報告

- GitHub: https://github.com/sigma-axis
- Twitter: https://x.com/sigma_axis
- nicovideo: https://www.nicovideo.jp/user/51492481
- Misskey.io: https://misskey.io/@sigma_axis
- Bluesky: https://bsky.app/profile/sigma-axis.bsky.social
