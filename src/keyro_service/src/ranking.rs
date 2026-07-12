// Copyright (C) 2025-2026 株式会社LocalPro (LocalPro Co., Ltd.). All rights reserved.
// Brand Official Website: https://keyro.jp
//
// This file is part of KeyroIME (キーロ) v1.0 OpenCore.
// KeyroIME is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software Foundation.
//
// For commercial use licensing, custom deployment, or proprietary integrations,
// please contact 株式会社LocalPro via https://localpro.jp. Unauthorized closed-source
// commercial exploitation is strictly prohibited.
use std::collections::HashSet;

use crate::dictionary::{self, CandidateMatch, DictionaryCandidate, DictionaryKind};
use crate::protocol::PAGE_SIZE;
use crate::romaji::RomajiConverter;

const KEYRO_HELP_PROMPT: &str = "次ページでKeyroIMEヘルプを見る";
const KEYRO_HELP_PAGE: [&str; PAGE_SIZE] = [
    "KeyroIMEヘルプ",
    "keyro.jp",
    "機能｜ショートカット",
    "入力モード: Alt+~ / 文字幅: Shift+Caps",
    "句読点: Shift / 配列: Alt+;",
];
const USER_EXACT_MATCH_BOOST: u32 = 24_000;
const USER_SOURCE_BOOST: u32 = 30_000;

#[derive(Debug, Clone, Copy, Eq, PartialEq)]
pub enum DictMode {
    Normal,
    QPrefix,
    VPrefix,
}

#[derive(Debug, Clone, Copy, Eq, PartialEq)]
enum CandidateSource {
    User,
    System,
    Translation,
    Name,
    Place,
    Station,
    Generated,
}

#[derive(Debug, Clone)]
struct RawCandidate {
    text: String,
    base_score: u32,
    source: CandidateSource,
    match_kind: RankedMatch,
}

#[derive(Debug, Clone, Copy, Eq, PartialEq)]
enum RankedMatch {
    Exact,
    Prefix,
    Middle,
    Generated,
    Fallback,
}

#[derive(Debug, Clone, Eq, PartialEq)]
pub struct RankedPage {
    pub candidates: Vec<String>,
    pub total_pages: usize,
}

pub struct CandidateSorter;

impl CandidateSorter {
    pub fn rank_page(input: &str, page: usize) -> RankedPage {
        let ranked = rank_all(input);
        let total_pages = ((ranked.len() + PAGE_SIZE - 1) / PAGE_SIZE).max(1);
        let safe_page = page.min(total_pages.saturating_sub(1));
        let start = safe_page.saturating_mul(PAGE_SIZE);

        RankedPage {
            candidates: ranked.into_iter().skip(start).take(PAGE_SIZE).collect(),
            total_pages,
        }
    }

    pub fn re_rank(input: &str, page: usize) -> Vec<String> {
        Self::rank_page(input, page).candidates
    }
}

fn rank_all(input: &str) -> Vec<String> {
    if input.is_empty() {
        return Vec::new();
    }

    let (query, mode) = if let Some(query) = input.strip_prefix('q') {
        (query, DictMode::QPrefix)
    } else if let Some(query) = input.strip_prefix('v') {
        (query, DictMode::VPrefix)
    } else {
        (input, DictMode::Normal)
    };

    let converter = RomajiConverter::new();
    let kana = converter.convert(query);
    let mut raw_candidates = match mode {
        DictMode::Normal => {
            let mut candidates = system_candidates(&kana, query, true);
            candidates.extend(translation_candidates(query, &kana));
            candidates.extend(specialized_candidates(&kana));
            candidates
        }
        DictMode::QPrefix => {
            let mut candidates = translation_candidates(query, &kana);
            candidates.extend(translation_predictive_candidates(query, &kana));
            candidates.extend(system_candidates(&kana, query, false));
            candidates
        }
        DictMode::VPrefix => {
            let mut candidates = specialized_candidates(&kana);
            candidates.extend(specialized_predictive_candidates(&kana));
            candidates.extend(system_candidates(&kana, query, false));
            candidates
        }
    };

    sort_ranked_candidates(&mut raw_candidates, mode);
    let mut seen_surfaces = HashSet::new();
    raw_candidates.retain(|candidate| {
        candidate.text.is_empty() || seen_surfaces.insert(candidate.text.clone())
    });

    let ranked = match mode {
        DictMode::Normal => raw_candidates,
        DictMode::QPrefix => inject_per_page(raw_candidates, is_translation, 3),
        DictMode::VPrefix => inject_per_page(raw_candidates, is_specialized, 4),
    };

    let formatted = ranked.into_iter().map(format_candidate).collect::<Vec<_>>();
    if mode == DictMode::Normal && is_keyro_help_query(query, &kana) {
        return inject_keyro_help_page(formatted);
    }
    formatted
}

fn is_keyro_help_query(query: &str, kana: &str) -> bool {
    normalize_keyro_query(query) == "きーろ" || normalize_keyro_query(kana) == "きーろ"
}

fn normalize_keyro_query(value: &str) -> String {
    value
        .chars()
        .map(|character| match character {
            'ァ'..='ン' | 'ヴ' => char::from_u32(character as u32 - 0x60).unwrap_or(character),
            'ｰ' | '－' | '-' => 'ー',
            _ => character.to_ascii_lowercase(),
        })
        .collect()
}

fn inject_keyro_help_page(candidates: Vec<String>) -> Vec<String> {
    let reserved = std::iter::once(KEYRO_HELP_PROMPT)
        .chain(KEYRO_HELP_PAGE.iter().copied())
        .collect::<HashSet<_>>();
    let mut result = Vec::with_capacity(PAGE_SIZE * 2);
    let mut seen = HashSet::new();

    for candidate in candidates {
        if candidate.is_empty() || reserved.contains(candidate.as_str()) {
            continue;
        }
        if seen.insert(candidate.clone()) {
            result.push(candidate);
        }
        if result.len() == PAGE_SIZE - 1 {
            break;
        }
    }

    for fallback in ["KeyroIME", "キーロ", "キーロ入力", "keyro.jp"] {
        if result.len() == PAGE_SIZE - 1 {
            break;
        }
        if seen.insert(fallback.to_string()) {
            result.push(fallback.to_string());
        }
    }

    while result.len() < PAGE_SIZE - 1 {
        result.push(String::new());
    }
    result.push(KEYRO_HELP_PROMPT.to_string());
    result.extend(
        KEYRO_HELP_PAGE
            .iter()
            .map(|candidate| candidate.to_string()),
    );
    result
}

fn weighted_score(candidate: &RawCandidate, mode: DictMode) -> u32 {
    let match_boost = match candidate.match_kind {
        RankedMatch::Exact => match candidate.source {
            CandidateSource::User => USER_EXACT_MATCH_BOOST,
            CandidateSource::System => 20_000,
            CandidateSource::Translation
            | CandidateSource::Name
            | CandidateSource::Place
            | CandidateSource::Station => 8_000,
            CandidateSource::Generated => 5_000,
        },
        RankedMatch::Prefix => 15_000,
        RankedMatch::Middle => 10_000,
        RankedMatch::Generated => 5_000,
        RankedMatch::Fallback => 0,
    };
    let source_boost = match candidate.source {
        CandidateSource::User => USER_SOURCE_BOOST,
        CandidateSource::Translation => {
            if mode == DictMode::QPrefix {
                5_000
            } else {
                1_000
            }
        }
        CandidateSource::Name | CandidateSource::Place | CandidateSource::Station => {
            if mode == DictMode::VPrefix {
                4_000
            } else {
                2_500
            }
        }
        CandidateSource::System | CandidateSource::Generated => 0,
    };
    candidate
        .base_score
        .saturating_add(match_boost)
        .saturating_add(source_boost)
}

fn sort_ranked_candidates(candidates: &mut [RawCandidate], mode: DictMode) {
    candidates.sort_by(|left, right| {
        weighted_score(right, mode)
            .cmp(&weighted_score(left, mode))
            .then_with(|| source_priority(right.source).cmp(&source_priority(left.source)))
            .then_with(|| match_priority(right.match_kind).cmp(&match_priority(left.match_kind)))
            .then_with(|| right.base_score.cmp(&left.base_score))
            .then_with(|| left.text.cmp(&right.text))
    });
}

fn source_priority(source: CandidateSource) -> u8 {
    match source {
        CandidateSource::User => 6,
        CandidateSource::Name | CandidateSource::Place | CandidateSource::Station => 5,
        CandidateSource::Translation => 4,
        CandidateSource::System => 3,
        CandidateSource::Generated => 2,
    }
}

fn match_priority(match_kind: RankedMatch) -> u8 {
    match match_kind {
        RankedMatch::Exact => 5,
        RankedMatch::Prefix => 4,
        RankedMatch::Middle => 3,
        RankedMatch::Generated => 2,
        RankedMatch::Fallback => 1,
    }
}

fn translation_candidates(query: &str, kana: &str) -> Vec<RawCandidate> {
    dictionary::translation_candidates(query, kana)
        .into_iter()
        .map(raw_from_dictionary)
        .collect()
}

fn translation_predictive_candidates(query: &str, kana: &str) -> Vec<RawCandidate> {
    dictionary::translation_predictive_candidates(query, kana)
        .into_iter()
        .map(raw_from_dictionary)
        .collect()
}

fn specialized_candidates(kana: &str) -> Vec<RawCandidate> {
    dictionary::specialized_candidates(kana)
        .into_iter()
        .map(raw_from_dictionary)
        .collect()
}

fn specialized_predictive_candidates(kana: &str) -> Vec<RawCandidate> {
    dictionary::specialized_predictive_candidates(kana)
        .into_iter()
        .map(raw_from_dictionary)
        .collect()
}

fn system_candidates(kana: &str, raw_input: &str, include_expansions: bool) -> Vec<RawCandidate> {
    let mut candidates = dictionary::system_candidates(kana)
        .into_iter()
        .map(raw_from_dictionary)
        .collect::<Vec<_>>();

    if include_expansions {
        candidates.extend(
            dictionary::predictive_candidates(kana)
                .into_iter()
                .map(raw_from_dictionary),
        );
        candidates.extend(
            dictionary::concatenated_candidates(kana)
                .into_iter()
                .map(raw_from_dictionary),
        );
    }

    if !kana.is_empty() {
        let fallback = if kana
            .chars()
            .any(|character| character.is_ascii_alphabetic())
        {
            raw_input
        } else {
            kana
        };
        candidates.push(raw(
            fallback,
            700,
            CandidateSource::System,
            RankedMatch::Fallback,
        ));
    } else if !raw_input.is_empty() {
        candidates.push(raw(
            raw_input,
            700,
            CandidateSource::System,
            RankedMatch::Fallback,
        ));
    }

    candidates
}

fn inject_per_page(
    candidates: Vec<RawCandidate>,
    is_promoted: fn(CandidateSource) -> bool,
    max_slots_per_page: usize,
) -> Vec<RawCandidate> {
    let promoted = candidates
        .iter()
        .filter(|candidate| is_promoted(candidate.source))
        .cloned()
        .collect::<Vec<_>>();
    let regular = candidates
        .into_iter()
        .filter(|candidate| !is_promoted(candidate.source))
        .collect::<Vec<_>>();

    let regular_slots = PAGE_SIZE - max_slots_per_page;
    let promoted_pages = (promoted.len() + max_slots_per_page - 1) / max_slots_per_page;
    let regular_pages = (regular.len() + regular_slots - 1) / regular_slots;
    let page_count = promoted_pages.max(regular_pages).max(1);
    let mut result = Vec::new();

    for page in 0..page_count {
        let promoted_start = page * max_slots_per_page;
        result.extend(
            promoted
                .iter()
                .skip(promoted_start)
                .take(max_slots_per_page)
                .cloned(),
        );

        let regular_start = page * regular_slots;
        result.extend(
            regular
                .iter()
                .skip(regular_start)
                .take(regular_slots)
                .cloned(),
        );
        while result.len() < (page + 1) * PAGE_SIZE {
            result.push(raw("", 0, CandidateSource::System, RankedMatch::Fallback));
        }
    }

    result
}

fn format_candidate(candidate: RawCandidate) -> String {
    if candidate.text.is_empty() {
        return String::new();
    }
    match candidate.source {
        CandidateSource::Translation => format!("[訳] {}", candidate.text),
        CandidateSource::Name => format!("[人名] {}", candidate.text),
        CandidateSource::Place => format!("[地名] {}", candidate.text),
        CandidateSource::Station => format!("[駅名] {}", candidate.text),
        CandidateSource::User | CandidateSource::System | CandidateSource::Generated => {
            candidate.text
        }
    }
}

fn raw_from_dictionary(candidate: DictionaryCandidate) -> RawCandidate {
    let source = match candidate.kind {
        DictionaryKind::User => CandidateSource::User,
        DictionaryKind::System => CandidateSource::System,
        DictionaryKind::Name => CandidateSource::Name,
        DictionaryKind::Place => CandidateSource::Place,
        DictionaryKind::Station => CandidateSource::Station,
        DictionaryKind::Translation => CandidateSource::Translation,
        DictionaryKind::Generated => CandidateSource::Generated,
    };
    RawCandidate {
        text: candidate.text,
        base_score: candidate.score,
        source,
        match_kind: match candidate.match_kind {
            CandidateMatch::Exact => RankedMatch::Exact,
            CandidateMatch::Prefix => RankedMatch::Prefix,
            CandidateMatch::Middle => RankedMatch::Middle,
            CandidateMatch::Generated => RankedMatch::Generated,
        },
    }
}

fn is_translation(source: CandidateSource) -> bool {
    source == CandidateSource::Translation
}

fn is_specialized(source: CandidateSource) -> bool {
    matches!(
        source,
        CandidateSource::Name | CandidateSource::Place | CandidateSource::Station
    )
}

fn raw(
    text: &str,
    base_score: u32,
    source: CandidateSource,
    match_kind: RankedMatch,
) -> RawCandidate {
    RawCandidate {
        text: text.to_string(),
        base_score,
        source,
        match_kind,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::time::{Duration, Instant};

    #[test]
    fn normal_input_is_converted_to_kana_before_lookup() {
        let page = CandidateSorter::rank_page("koukan", 0);
        assert_eq!(page.candidates[0], "交換");
        assert!(page.total_pages >= 2);
    }

    #[test]
    fn q_prefix_promotes_three_translation_candidates_on_each_page() {
        for page_index in 0..2 {
            let page = CandidateSorter::rank_page("qcheck", page_index);
            assert_eq!(page.candidates.len(), PAGE_SIZE);
            assert!(page.candidates[0].starts_with("[訳]"));
            assert!(page.candidates[1].starts_with("[訳]"));
            assert!(page.candidates[2].starts_with("[訳]"));
        }
    }

    #[test]
    fn v_prefix_promotes_four_place_candidates_on_each_page() {
        for page_index in 0..2 {
            let page = CandidateSorter::rank_page("vootuki", page_index);
            assert_eq!(page.candidates.len(), PAGE_SIZE);
            assert!(page.candidates[0].starts_with('['));
            assert!(page.candidates[1].starts_with('['));
            assert!(page.candidates[2].starts_with('['));
            assert!(page.candidates[3].starts_with('['));
        }
    }

    #[test]
    fn promoted_page_injection_preserves_every_regular_candidate() {
        let mut candidates = (0..3)
            .map(|index| {
                raw(
                    &format!("promoted-{index}"),
                    100,
                    CandidateSource::Translation,
                    RankedMatch::Exact,
                )
            })
            .collect::<Vec<_>>();
        candidates.extend((0..7).map(|index| {
            raw(
                &format!("regular-{index}"),
                100,
                CandidateSource::System,
                RankedMatch::Exact,
            )
        }));

        let injected = inject_per_page(candidates, is_translation, 3);
        for index in 0..7 {
            assert!(injected
                .iter()
                .any(|candidate| candidate.text == format!("regular-{index}")));
        }
    }

    #[test]
    fn static_assets_replace_generated_stub_candidates() {
        let page = CandidateSorter::rank_page("taberu", 0);
        assert_eq!(page.candidates[0], "食べる");
        assert!(!page.candidates.iter().any(|value| value == "たべる情報"));
    }

    #[test]
    fn translation_dictionary_supports_japanese_to_english() {
        let page = CandidateSorter::rank_page("qnihon", 0);
        assert_eq!(page.candidates[0], "[訳] Japan");
    }

    #[test]
    fn q_prefix_supports_product_translation_examples() {
        let schedule = CandidateSorter::rank_page("qスケジュール", 0);
        assert!(schedule
            .candidates
            .iter()
            .take(3)
            .any(|candidate| candidate == "[訳] schedule"));

        let schedule_romaji = CandidateSorter::rank_page("qsukeju-ru", 0);
        assert!(schedule_romaji
            .candidates
            .iter()
            .take(3)
            .any(|candidate| candidate == "[訳] schedule"));

        let schedule_english = CandidateSorter::rank_page("qschedule", 0);
        assert_eq!(schedule_english.candidates[0], "[訳] スケジュール");

        let digital = CandidateSorter::rank_page("qDigital Transformation", 0);
        assert_eq!(
            digital.candidates[0],
            "[訳] デジタルトランスフォーメーション"
        );

        let compact = CandidateSorter::rank_page("qdigitaltransformation", 0);
        assert_eq!(
            compact.candidates[0],
            "[訳] デジタルトランスフォーメーション"
        );
    }

    #[test]
    fn q_and_v_prefixes_predict_from_one_kana_or_kanji() {
        let q_kana = CandidateSorter::rank_page("qす", 0);
        assert!(q_kana
            .candidates
            .iter()
            .take(3)
            .any(|candidate| candidate == "[訳] schedule"));

        let q_kanji = CandidateSorter::rank_page("q日", 0);
        assert!(q_kanji
            .candidates
            .iter()
            .take(3)
            .any(|candidate| candidate == "[訳] Japan"));

        let v_kana = CandidateSorter::rank_page("vし", 0);
        assert!(v_kana
            .candidates
            .iter()
            .take(4)
            .any(|candidate| candidate.starts_with("[人名]")
                || candidate.starts_with("[地名]")
                || candidate.starts_with("[駅名]")));

        let v_kanji = CandidateSorter::rank_page("v新", 0);
        assert!(v_kanji
            .candidates
            .iter()
            .take(4)
            .any(|candidate| candidate == "[地名] 新宿"));
    }

    #[test]
    fn normal_input_includes_labeled_translation_and_place_sources() {
        let check = CandidateSorter::rank_page("check", 0);
        assert!(check
            .candidates
            .iter()
            .any(|candidate| candidate.starts_with("[訳]")));

        let shibuya = CandidateSorter::rank_page("shibuya", 0);
        assert!(shibuya
            .candidates
            .iter()
            .any(|candidate| candidate.starts_with("[地名]")));
    }

    #[test]
    fn english_translation_fallback_never_exposes_partial_romaji_conversion() {
        let page = CandidateSorter::rank_page("qcomputer", 0);
        assert!(page
            .candidates
            .iter()
            .any(|candidate| candidate == "computer"));
        assert!(!page
            .candidates
            .iter()
            .any(|candidate| candidate.contains("cおmぷてr")));
    }

    #[test]
    fn katakana_loanwords_are_mixed_into_normal_candidates() {
        let page = CandidateSorter::rank_page("insuto-ru", 0);
        assert!(page
            .candidates
            .iter()
            .any(|candidate| candidate == "インストール"));

        let modern_term = CandidateSorter::rank_page("zerotorasuto", 0);
        assert!(modern_term
            .candidates
            .iter()
            .any(|candidate| candidate == "ゼロトラスト"));
    }

    #[test]
    fn keyro_katakana_query_inserts_help_prompt_as_fifth_candidate() {
        let page = CandidateSorter::rank_page("キーロ", 0);
        assert_eq!(page.total_pages, 2);
        assert_eq!(page.candidates.len(), PAGE_SIZE);
        assert_eq!(page.candidates[PAGE_SIZE - 1], KEYRO_HELP_PROMPT);

        let romaji_page = CandidateSorter::rank_page("ki-ro", 0);
        assert_eq!(romaji_page.total_pages, 2);
        assert_eq!(romaji_page.candidates[PAGE_SIZE - 1], KEYRO_HELP_PROMPT);
    }

    #[test]
    fn keyro_help_page_replaces_second_candidate_page() {
        let page = CandidateSorter::rank_page("キーロ", 1);
        assert_eq!(
            page.candidates,
            KEYRO_HELP_PAGE
                .iter()
                .map(|candidate| candidate.to_string())
                .collect::<Vec<_>>()
        );
        assert!(page
            .candidates
            .iter()
            .any(|candidate| candidate == "keyro.jp"));
        assert!(page
            .candidates
            .iter()
            .any(|candidate| candidate.contains("Alt+~")));
        assert!(page
            .candidates
            .iter()
            .any(|candidate| candidate.contains("Shift+Caps")));
        assert!(page
            .candidates
            .iter()
            .any(|candidate| candidate.contains("Shift") && candidate.contains("Alt+;")));
    }

    #[test]
    fn exact_candidates_precede_prefix_and_middle_predictions() {
        let candidates = rank_all("shouchi");
        let exact = candidates
            .iter()
            .position(|candidate| candidate == "承知")
            .expect("exact candidate");
        let prefix = candidates
            .iter()
            .position(|candidate| candidate == "承知しました")
            .expect("prefix candidate");
        assert!(
            exact < prefix || prefix == 0,
            "learned user selections may outrank the static exact candidate"
        );
    }

    #[test]
    fn user_prediction_outranks_static_exact_candidate() {
        let mut candidates = vec![
            raw(
                "静的候補",
                4_000,
                CandidateSource::System,
                RankedMatch::Exact,
            ),
            raw(
                "ユーザー候補",
                2_000,
                CandidateSource::User,
                RankedMatch::Prefix,
            ),
        ];
        sort_ranked_candidates(&mut candidates, DictMode::Normal);
        assert_eq!(candidates[0].text, "ユーザー候補");
    }

    #[test]
    fn one_character_kana_and_kanji_queries_return_predictions() {
        let kana = rank_all("tu");
        assert!(kana.iter().any(|candidate| candidate == "使う"));

        let kanji = rank_all("承");
        assert!(kanji.iter().any(|candidate| candidate == "承知"));
        assert!(kanji.iter().any(|candidate| candidate == "承知しました"));
    }

    #[test]
    fn predictive_completion_recalls_prefix_and_middle_phrases() {
        let tsuka = rank_all("tsuka");
        for expected in [
            "使う",
            "遣う",
            "使い",
            "疲れ",
            "お疲れ様",
            "お疲れ様です",
            "お疲れ様でした",
        ] {
            assert!(
                tsuka.iter().any(|candidate| candidate == expected),
                "missing {expected}"
            );
        }
        let prefix_position = tsuka
            .iter()
            .position(|candidate| candidate == "使う")
            .expect("prefix candidate");
        let middle_position = tsuka
            .iter()
            .position(|candidate| candidate == "お疲れ様")
            .expect("middle candidate");
        assert!(prefix_position < middle_position);
        for phrase in ["お疲れ様", "お疲れ様です", "お疲れ様でした"] {
            assert!(
                tsuka
                    .iter()
                    .position(|candidate| candidate == phrase)
                    .expect("middle phrase")
                    < 15,
                "{phrase} should appear in the first three pages"
            );
        }

        let shouchi = rank_all("shouchi");
        assert!(shouchi.iter().any(|candidate| candidate == "承知"));
        assert!(shouchi.iter().any(|candidate| candidate == "承知しました"));
        assert!(shouchi
            .iter()
            .any(|candidate| candidate == "承知いたしました"));
    }

    #[test]
    fn generated_compounds_follow_native_full_reading_candidates() {
        let candidates = rank_all("shiyousho");
        let native = candidates
            .iter()
            .position(|candidate| candidate == "仕様書")
            .expect("native full-reading candidate");
        let generated = candidates
            .iter()
            .position(|candidate| candidate == "使用書")
            .expect("generated compound candidate");
        assert!(native < generated);
    }

    #[test]
    fn expanded_dictionary_warm_lookup_stays_within_ten_milliseconds() {
        dictionary::warm_up();
        let queries = [
            "koukan",
            "kakunin",
            "keizai",
            "taberu",
            "qcomputer",
            "qsoftware",
            "vshibuya",
            "vsatou",
            "tsuka",
            "shouchi",
            "shiyousho",
            "insuto-ru",
        ];
        let mut samples = Vec::with_capacity(queries.len() * 25);
        for _ in 0..25 {
            for query in queries {
                let start = Instant::now();
                let page = CandidateSorter::rank_page(query, 0);
                samples.push(start.elapsed());
                assert!(!page.candidates.is_empty());
            }
        }
        samples.sort_unstable();
        let percentile_95 = samples[(samples.len() * 95 / 100).min(samples.len() - 1)];
        assert!(
            percentile_95 < Duration::from_millis(10),
            "warm lookup p95 was {percentile_95:?}"
        );
    }
}
