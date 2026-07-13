// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) OpenCore.
// It is source-available under the KeyroIME OpenCore Non-Commercial Source
// License 1.0. See LICENSE. Commercial use requires a separate written license
// from 株式会社LocalPro.
/// ImeStateMachine 日本語入力コア状態マシン
/// Empty、Composing、DeepNavigating の3状態遷移を管理します。
/// 日英混在入力の Fallback と接頭辞フィルタを実装します。
use crate::romaji::RomajiConverter;

/// 入力状態列挙型
#[derive(Debug, Clone, PartialEq)]
pub enum ImeState {
    /// 待機状態: 入力バッファーがなく、候補ウィンドウも表示されていません。
    Empty,
    /// 合成状態: 初期入力中で、候補ウィンドウは表示済みだがナビゲーションは未開始です。
    Composing,
    /// 深いナビゲーション状態: ページ移動またはカーソル移動が開始されています。
    DeepNavigating,
}

/// 接頭辞フィルタモード
#[derive(Debug, Clone, PartialEq)]
pub enum PrefixMode {
    /// 通常モード
    Normal,
    /// q 接頭辞: 日英/英日翻訳モード（各ページ先頭3件に翻訳候補を優先挿入）。
    QPrefix,
    /// v 接頭辞: 人名・地名・駅名の専用辞書（各ページ先頭4件に優先挿入）。
    VPrefix,
}

/// 状態マシン処理結果
#[derive(Debug, Clone)]
pub struct ImeResult {
    /// コミットが必要なテキスト（None はコミットなし）。
    pub commit_text: Option<String>,
    /// 更新後の候補一覧（空の場合は候補ウィンドウを閉じます）。
    pub updated_candidates: Vec<String>,
    /// 現在のかなバッファー（UI の下線表示に使用）。
    pub current_kana: String,
}

/// 入力状態マシン
pub struct ImeStateMachine {
    /// 現在状態
    pub state: ImeState,
    /// ローマ字入力バッファー
    pub romaji_buffer: String,
    /// 現在変換済みのかな
    pub kana_buffer: String,
    /// 候補プール（全候補）
    pub candidates_pool: Vec<String>,
    /// 現在ページ番号（0始まり）
    pub current_page: usize,
    /// 現在のハイライト索引（現在ページ内の相対索引 0-4）
    pub highlight_index: usize,
    /// 英文 Fallback フラグ
    pub is_english_fallback: bool,
    /// 接頭辞モード
    pub prefix_mode: PrefixMode,
    /// ローマ字変換器
    converter: RomajiConverter,
}

impl ImeStateMachine {
    /// 新しい状態マシンインスタンスを作成します。
    pub fn new() -> Self {
        ImeStateMachine {
            state: ImeState::Empty,
            romaji_buffer: String::new(),
            kana_buffer: String::new(),
            candidates_pool: Vec::new(),
            current_page: 0,
            highlight_index: 0,
            is_english_fallback: false,
            prefix_mode: PrefixMode::Normal,
            converter: RomajiConverter::new(),
        }
    }

    /// 処理文字キー入力
    pub fn handle_char(&mut self, ch: char) -> ImeResult {
        match self.state {
            ImeState::Empty => {
                // Empty 状態: 新しい入力を開始します。
                if ch.is_ascii_digit() {
                    // 数字は候補を出さずに直接コミットします。
                    return ImeResult {
                        commit_text: Some(ch.to_string()),
                        updated_candidates: Vec::new(),
                        current_kana: String::new(),
                    };
                }

                // ローマ字バッファーへ追加します。
                self.romaji_buffer.push(ch);

                // 接頭辞モードを検出します。
                self.detect_prefix_mode();

                // かなへ変換します。
                self.update_kana();

                // English Fallback を発動するか評価します。
                self.evaluate_english_fallback();

                // 候補を取得します（模擬）。
                self.fetch_candidates();

                // Composing 状態へ切り替えます。
                self.state = ImeState::Composing;
                self.highlight_index = 0;

                ImeResult {
                    commit_text: None,
                    updated_candidates: self.get_current_page_candidates(),
                    current_kana: self.kana_buffer.clone(),
                }
            }

            ImeState::Composing => {
                // Composing 状態: 数字キー1-5を候補選択として扱います。
                if ch.is_ascii_digit() && ('1'..='5').contains(&ch) {
                    let idx = (ch.to_digit(10).unwrap() - 1) as usize;
                    return self.select_candidate_by_index(idx);
                }

                // 文字を続けて追加します。
                self.romaji_buffer.push(ch);
                self.update_kana();
                self.evaluate_english_fallback();
                self.fetch_candidates();

                ImeResult {
                    commit_text: None,
                    updated_candidates: self.get_current_page_candidates(),
                    current_kana: self.kana_buffer.clone(),
                }
            }

            ImeState::DeepNavigating => {
                // DeepNavigating 状態: 数字キーで現在ページの候補を選択します。
                if ch.is_ascii_digit() && ('1'..='5').contains(&ch) {
                    let idx = (ch.to_digit(10).unwrap() - 1) as usize;
                    return self.select_candidate_by_index(idx);
                }

                // その他の文字: 現在のハイライト候補を先にコミットし、新しい入力を開始します。
                let commit = self.get_highlighted_candidate();
                self.reset();

                // 新しい文字を再帰処理します。
                let mut new_result = self.handle_char(ch);
                new_result.commit_text = Some(commit);
                new_result
            }
        }
    }

    /// Enter キーを処理します。
    pub fn handle_enter(&mut self) -> ImeResult {
        let commit_text = match self.state {
            ImeState::Empty => {
                // Empty 状態の Enter: 改行を挿入します。
                "\n".to_string()
            }
            ImeState::Composing => {
                // Composing 状態: 元のかな、または Fallback 英語をコミットします。
                if self.is_english_fallback {
                    self.romaji_buffer.clone()
                } else {
                    self.kana_buffer.clone()
                }
            }
            ImeState::DeepNavigating => {
                // DeepNavigating 状態: 現在のハイライト候補をコミットします。
                self.get_highlighted_candidate()
            }
        };

        self.reset();
        ImeResult {
            commit_text: Some(commit_text),
            updated_candidates: Vec::new(),
            current_kana: String::new(),
        }
    }

    /// Space キーを処理します。
    pub fn handle_space(&mut self) -> ImeResult {
        match self.state {
            ImeState::Empty => {
                // Empty 状態の Space: 空白を直接コミットします。
                ImeResult {
                    commit_text: Some(" ".to_string()),
                    updated_candidates: Vec::new(),
                    current_kana: String::new(),
                }
            }
            ImeState::Composing | ImeState::DeepNavigating => {
                // コミット現在ハイライト候補
                let commit_text = if self.state == ImeState::Composing && self.highlight_index == 0
                {
                    // Composing の既定ハイライトは1件目です。
                    self.candidates_pool
                        .get(0)
                        .cloned()
                        .unwrap_or(self.kana_buffer.clone())
                } else {
                    self.get_highlighted_candidate()
                };

                self.reset();
                ImeResult {
                    commit_text: Some(commit_text),
                    updated_candidates: Vec::new(),
                    current_kana: String::new(),
                }
            }
        }
    }

    /// 下矢印（または Tab。v1.0 では Tab は ↓ と同等）を処理します。
    pub fn handle_down(&mut self) -> ImeResult {
        match self.state {
            ImeState::Empty => {
                // 空状態では応答しません。
                ImeResult {
                    commit_text: None,
                    updated_candidates: Vec::new(),
                    current_kana: String::new(),
                }
            }
            ImeState::Composing => {
                // Composing から DeepNavigating へ遷移します。
                self.state = ImeState::DeepNavigating;
                self.highlight_index = (self.highlight_index + 1) % 5;

                ImeResult {
                    commit_text: None,
                    updated_candidates: self.get_current_page_candidates(),
                    current_kana: self.kana_buffer.clone(),
                }
            }
            ImeState::DeepNavigating => {
                // ハイライトを循環移動します。
                self.highlight_index = (self.highlight_index + 1) % 5;

                ImeResult {
                    commit_text: None,
                    updated_candidates: self.get_current_page_candidates(),
                    current_kana: self.kana_buffer.clone(),
                }
            }
        }
    }

    /// 処理上矢印
    pub fn handle_up(&mut self) -> ImeResult {
        match self.state {
            ImeState::Empty => ImeResult {
                commit_text: None,
                updated_candidates: Vec::new(),
                current_kana: String::new(),
            },
            ImeState::Composing => {
                self.state = ImeState::DeepNavigating;
                self.highlight_index = 4; // 5件目へ循環します。

                ImeResult {
                    commit_text: None,
                    updated_candidates: self.get_current_page_candidates(),
                    current_kana: self.kana_buffer.clone(),
                }
            }
            ImeState::DeepNavigating => {
                self.highlight_index = if self.highlight_index == 0 {
                    4
                } else {
                    self.highlight_index - 1
                };

                ImeResult {
                    commit_text: None,
                    updated_candidates: self.get_current_page_candidates(),
                    current_kana: self.kana_buffer.clone(),
                }
            }
        }
    }

    /// ページ送り（Page Down または . キー）を処理します。
    pub fn handle_page_down(&mut self) -> ImeResult {
        if self.state == ImeState::Empty {
            return ImeResult {
                commit_text: None,
                updated_candidates: Vec::new(),
                current_kana: String::new(),
            };
        }

        if self.state == ImeState::Composing {
            self.state = ImeState::DeepNavigating;
        }

        let max_page = (self.candidates_pool.len().saturating_sub(1)) / 5;
        if self.current_page < max_page {
            self.current_page += 1;
            self.highlight_index = 0;
        }

        ImeResult {
            commit_text: None,
            updated_candidates: self.get_current_page_candidates(),
            current_kana: self.kana_buffer.clone(),
        }
    }

    /// ページ戻し（Page Up または , キー）を処理します。
    pub fn handle_page_up(&mut self) -> ImeResult {
        if self.state == ImeState::Empty {
            return ImeResult {
                commit_text: None,
                updated_candidates: Vec::new(),
                current_kana: String::new(),
            };
        }

        if self.state == ImeState::Composing {
            self.state = ImeState::DeepNavigating;
        }

        if self.current_page > 0 {
            self.current_page -= 1;
            self.highlight_index = 0;
        }

        ImeResult {
            commit_text: None,
            updated_candidates: self.get_current_page_candidates(),
            current_kana: self.kana_buffer.clone(),
        }
    }

    /// 接頭辞モードを検出します。
    fn detect_prefix_mode(&mut self) {
        if self.romaji_buffer.starts_with('q') {
            self.prefix_mode = PrefixMode::QPrefix;
        } else if self.romaji_buffer.starts_with('v') {
            self.prefix_mode = PrefixMode::VPrefix;
        } else {
            self.prefix_mode = PrefixMode::Normal;
        }
    }

    /// 更新かなバッファー
    fn update_kana(&mut self) {
        // 接頭辞モードの場合は先頭文字を除いて変換します。
        let convert_input = match self.prefix_mode {
            PrefixMode::QPrefix | PrefixMode::VPrefix => {
                if self.romaji_buffer.len() > 1 {
                    &self.romaji_buffer[1..]
                } else {
                    ""
                }
            }
            PrefixMode::Normal => &self.romaji_buffer,
        };

        self.kana_buffer = self.converter.convert(convert_input);
    }

    /// English Fallback を発動するか評価します。
    fn evaluate_english_fallback(&mut self) {
        if self.prefix_mode != PrefixMode::Normal {
            self.is_english_fallback = false;
            return;
        }

        // すべて ASCII 文字で構成されているか確認します。
        let is_pure_alpha = self.romaji_buffer.chars().all(|c| c.is_ascii_alphabetic());

        // 模擬: 英単語（例: github）は日本語辞書での一致度を0とみなします。
        let has_zero_matches = self.is_english_word(&self.romaji_buffer);

        self.is_english_fallback = is_pure_alpha && has_zero_matches;
    }

    /// 英単語かどうかを模擬判定します（本来は辞書照会）。
    fn is_english_word(&self, word: &str) -> bool {
        // 簡易ヒューリスティック：よく使う英文語彙
        matches!(
            word.to_lowercase().as_str(),
            "github" | "google" | "hello" | "world" | "test" | "code"
        )
    }

    /// 取得候補（模擬実装）
    fn fetch_candidates(&mut self) {
        self.candidates_pool.clear();

        match self.prefix_mode {
            PrefixMode::QPrefix => {
                // q モード: 先頭3件に翻訳候補を挿入します（タグ: [訳]）。
                self.candidates_pool.push("[訳] check".to_string());
                self.candidates_pool.push("[訳] チェック".to_string());
                self.candidates_pool.push("[訳] 検査".to_string());
                self.candidates_pool.push(self.kana_buffer.clone());
                self.candidates_pool.push("系統詞1".to_string());
            }
            PrefixMode::VPrefix => {
                // v モード: 先頭4件に固有名詞を挿入し、表示タグを [名] に統一します。
                self.candidates_pool.push("[名] 大塚愛".to_string());
                self.candidates_pool.push("[名] 大塚駅".to_string());
                self.candidates_pool.push("[名] 大塚".to_string());
                self.candidates_pool.push("[名] Ōtsuka".to_string());
                self.candidates_pool.push(self.kana_buffer.clone());
            }
            PrefixMode::Normal => {
                // 通常モード: かなと模擬漢字候補を返します。
                if !self.kana_buffer.is_empty() {
                    self.candidates_pool.push(self.kana_buffer.clone());
                    self.candidates_pool.push("交換".to_string());
                    self.candidates_pool.push("光環".to_string());
                    self.candidates_pool.push("交歓".to_string());
                    self.candidates_pool.push("後間".to_string());
                }
            }
        }
    }

    /// 現在ページの候補を取得します（各ページ5件）。
    fn get_current_page_candidates(&self) -> Vec<String> {
        let start = self.current_page * 5;
        self.candidates_pool
            .iter()
            .skip(start)
            .take(5)
            .cloned()
            .collect()
    }

    /// 【公開インターフェイス】指定ページの候補を取得します（FFI 用）。
    /// 戻り値形式: 1ページ5件固定。足りない分は空文字列で埋めます。
    pub fn get_candidates_for_page(&self, page: usize) -> Vec<String> {
        let start = page * 5;
        let mut result = Vec::with_capacity(5);

        for i in 0..5 {
            let global_idx = start + i;
            if let Some(candidate) = self.candidates_pool.get(global_idx) {
                result.push(candidate.clone());
            } else {
                result.push(String::new()); // 5件に満たない場合は空文字列で埋めます。
            }
        }

        result
    }

    /// 【公開インターフェイス】候補総数を取得します。
    pub fn get_total_candidates_count(&self) -> usize {
        self.candidates_pool.len()
    }

    /// 【公開インターフェイス】総ページ数を取得します。
    pub fn get_total_pages(&self) -> usize {
        (self.candidates_pool.len() + 4) / 5 // 切り上げ
    }

    /// 現在ページ内の索引に基づいて候補を選択します。
    fn select_candidate_by_index(&mut self, page_index: usize) -> ImeResult {
        let global_index = self.current_page * 5 + page_index;
        let commit_text = self
            .candidates_pool
            .get(global_index)
            .cloned()
            .unwrap_or(self.kana_buffer.clone());

        self.reset();
        ImeResult {
            commit_text: Some(commit_text),
            updated_candidates: Vec::new(),
            current_kana: String::new(),
        }
    }

    /// 現在ハイライト中の候補を取得します。
    fn get_highlighted_candidate(&self) -> String {
        let global_index = self.current_page * 5 + self.highlight_index;
        self.candidates_pool
            .get(global_index)
            .cloned()
            .unwrap_or(self.kana_buffer.clone())
    }

    /// 状態マシンをリセットします。
    fn reset(&mut self) {
        self.state = ImeState::Empty;
        self.romaji_buffer.clear();
        self.kana_buffer.clear();
        self.candidates_pool.clear();
        self.current_page = 0;
        self.highlight_index = 0;
        self.is_english_fallback = false;
        self.prefix_mode = PrefixMode::Normal;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_basic_input() {
        let mut sm = ImeStateMachine::new();

        sm.handle_char('k');
        assert_eq!(sm.state, ImeState::Composing);
        assert_eq!(sm.kana_buffer, "k");

        sm.handle_char('o');
        assert_eq!(sm.kana_buffer, "こ");
    }

    #[test]
    fn test_composite_youon_mapping() {
        for (romaji, expected) in [
            ("jya", "じゃ"),
            ("jyu", "じゅ"),
            ("jyo", "じょ"),
            ("lya", "りゃ"),
        ] {
            let mut sm = ImeStateMachine::new();
            for ch in romaji.chars() {
                sm.handle_char(ch);
            }
            assert_eq!(sm.kana_buffer, expected, "failed to compose {romaji}");
        }
    }

    #[test]
    fn test_english_fallback() {
        let mut sm = ImeStateMachine::new();

        for ch in "github".chars() {
            sm.handle_char(ch);
        }

        assert!(sm.is_english_fallback);

        let result = sm.handle_enter();
        assert_eq!(result.commit_text, Some("github".to_string()));
    }

    #[test]
    fn test_q_prefix_mode() {
        let mut sm = ImeStateMachine::new();

        sm.handle_char('q');
        assert_eq!(sm.prefix_mode, PrefixMode::QPrefix);

        let candidates = sm.get_current_page_candidates();
        assert!(candidates.len() >= 3);
        assert!(candidates[0].contains("訳")); // 日本語タグへ修正済み
    }

    #[test]
    fn test_v_prefix_mode() {
        let mut sm = ImeStateMachine::new();

        sm.handle_char('v');
        assert_eq!(sm.prefix_mode, PrefixMode::VPrefix);

        let candidates = sm.get_current_page_candidates();
        assert!(candidates.len() >= 4);
    }

    #[test]
    fn test_number_direct_commit() {
        let mut sm = ImeStateMachine::new();

        let result = sm.handle_char('5');
        assert_eq!(result.commit_text, Some("5".to_string()));
        assert_eq!(sm.state, ImeState::Empty);
    }

    #[test]
    fn test_candidate_selection() {
        let mut sm = ImeStateMachine::new();

        sm.handle_char('k');
        sm.handle_char('o');

        let result = sm.handle_char('1');
        assert!(result.commit_text.is_some());
        assert_eq!(sm.state, ImeState::Empty);
    }

    #[test]
    fn test_navigation() {
        let mut sm = ImeStateMachine::new();

        sm.handle_char('k');
        sm.handle_char('o');
        assert_eq!(sm.state, ImeState::Composing);

        sm.handle_down();
        assert_eq!(sm.state, ImeState::DeepNavigating);
        assert_eq!(sm.highlight_index, 1);
    }
}
