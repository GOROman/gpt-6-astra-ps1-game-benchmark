# FACET FIGHTER — PlayStation 1

[![Development progress: 60% (6/10 verified milestones)](docs/progress.svg)](docs/STATUS.md)

**進捗: 60% — 10項目中6項目を検証済み。** PS1 ビルドと人体の描画を確認。SPU音声の録音を確認。操作・対戦の受け入れ検証を進行中。完成後にタグ・Release を公開します。

**開発モデル: GPT-6 Astra** — Codex 上で段階的に開発し、タスク単位で commit & push しています。ユーザーの指示履歴は [PROMPTS.md](PROMPTS.md) に時刻付きで記録します。

An original flat-shaded 3D fighting game for the original PlayStation, inspired by early polygonal arcade fighters. All fighters, geometry and presentation are original.

## Planned game

- Articulated low-poly fighters on an open, raised square arena.
- Punch / kick / guard controls, crouching, jumping, throws and high/mid/low attacks.
- Knockouts, ring-outs, 30-second rounds, first to two round wins; health decides time-outs and equal health draws replay.
- CPU arcade matches and local two-controller versus, character selection, pause and rematch.
- Native PS1 executable and BIN/CUE disc image, procedural sound effects.

## Development steps

1. Repository and explicit acceptance criteria.
2. Portable deterministic combat simulation and rule tests.
3. Native PS1 renderer, animated polygon fighters, controls and match menus.
4. Sound, tuning and integration verification.
5. Emulator acceptance run, distributable image and documented results.

Each completed step is committed and pushed separately. Completion is tracked in [docs/STATUS.md](docs/STATUS.md); the specification above is the target, not a claim that it is all implemented.

## Tools

PS1 code uses [PSn00bSDK](https://github.com/Lameguy64/PSn00bSDK). PS1 builds run on GitHub Actions. Portable rule tests run on the development machine using a C compiler.

No BIOS, commercial game assets or SDK binaries are included.

Run portable rule tests with `./tools/test.sh`.

## 最新の実行画面

![継続VBlank計測中のPS1対戦画面](docs/screenshots/006-vblank-audit.png)

GitHub Actions の `83a2c80` を既存のDuckStationで読み直して実行。表示は **59.82 FPS** ですが、継続計測では4,200描画フレーム中9回の2VBlank間隔を検出しました。診断ログの負荷も含めて調査中で、60fps維持の受け入れは未完了です。[計測ログ](docs/frame-audit.txt)。実機確認とは区別しています。

[スクリーンショット付き実装履歴](docs/IMPLEMENTATION.md)

## 操作

| ボタン | 操作 |
|---|---|
| 左右 | 移動 |
| 下 / 上 | しゃがむ / ジャンプ |
| □ / △ / × | パンチ / キック / ガード |
| ○ または □+× | 投げ |
| 下+△ / □+△ | 下段キック / ヘビーキック |
| R2 | 鉄山靠系ショルダータックル（中段・踏み込み・ダウン） |
| L1 / R1 | 相手の周りを移動 |
| L2 / 後ろ方向2回 | バックステップ |
| START | 決定 / ポーズ |
| ポーズ中 SELECT | タイトルへ戻る |

立ちガードは上・中段、しゃがみガードは下段を防ぎます。投げは立ちガードを崩し、しゃがみで回避できます。ショルダータックルは近距離から踏み込み、相手を大きく押し出します。立ちガードで防げます。

## ビルド・起動

PS1 ビルドは GitHub Actions の **PS1 build** で行います。Push ごとに戦闘テストを実行し、固定版 PSn00bSDK v0.24 で `facet.exe` と `facet.bin` / `facet.cue` を生成します。

```sh
# ローカルで戦闘ルールを検証
./tools/test.sh

# 手動で GitHub Actions を起動
gh workflow run build.yml
gh run list --workflow build.yml

# 成功した run ID の成果物を取得（ID を実際の番号に置き換える）
gh run download RUN_ID -n facet-ps1 -D build/download
```

`facet.cue` を、同じフォルダに `facet.bin` を置いた状態で DuckStation に読み込みます。更新時は既存の1インスタンスで再読み込み／ゲーム再起動を使用します。完成後に `v*` タグを作成すると、Actions がタグのソースを再ビルドして Release に ZIP・BIN/CUE・EXE・チェックサムを公開します。現時点では受け入れ検証中です。

[エミュレーターで録音した効果音（18秒・FLAC）](docs/audio/facet-impact-demo.flac)

打撃時は3フレーム、強打は6フレームのヒットストップ。ガード時は2フレーム。接触位置の火花とダメージに応じた画面揺れを表示します。構え・しゃがみ・攻撃は関節角度を補間し、手足の長さを維持します。
