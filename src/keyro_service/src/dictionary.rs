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
use std::collections::{HashMap, HashSet};
use std::env;
use std::fs::{self, OpenOptions};
use std::io::Write;
use std::path::PathBuf;
use std::sync::mpsc::{self, Sender};
use std::sync::{OnceLock, RwLock};
use std::thread;

use crate::romaji::RomajiConverter;

const SYSTEM_TSV: &str = include_str!(concat!(env!("KEYROIME_ASSET_DIR"), "/system.tsv"));
const FREQUENT_TSV: &str = include_str!(concat!(env!("KEYROIME_ASSET_DIR"), "/frequent.tsv"));
const NAME_TSV: &str = include_str!(concat!(env!("KEYROIME_ASSET_DIR"), "/names.tsv"));
const PLACE_TSV: &str = include_str!(concat!(env!("KEYROIME_ASSET_DIR"), "/places.tsv"));
const TRANSLATION_TSV: &str =
    include_str!(concat!(env!("KEYROIME_ASSET_DIR"), "/translations.tsv"));
const SYSTEM_SUPPLEMENT_TSV: &str = include_str!(concat!(
    env!("KEYROIME_ASSET_DIR"),
    "/system_supplement.tsv"
));
const NAME_SUPPLEMENT_TSV: &str =
    include_str!(concat!(env!("KEYROIME_ASSET_DIR"), "/names_supplement.tsv"));
const PLACE_SUPPLEMENT_TSV: &str = include_str!(concat!(
    env!("KEYROIME_ASSET_DIR"),
    "/places_supplement.tsv"
));
const TRANSLATION_SUPPLEMENT_TSV: &str = include_str!(concat!(
    env!("KEYROIME_ASSET_DIR"),
    "/translations_supplement.tsv"
));
const KATAKANA_TSV: &str = include_str!(concat!(env!("KEYROIME_ASSET_DIR"), "/katakana.tsv"));

const MAX_PREDICTIVE_CANDIDATES: usize = 96;
const MAX_PREFIX_PREDICTIVE_CANDIDATES: usize = 8;
const MAX_CONCATENATED_CANDIDATES: usize = 24;
const MAX_CONCATENATION_PARTS: usize = 3;
const MAX_FRAGMENT_POSTINGS: usize = 1024;
const MAX_USER_PREDICTIVE_CANDIDATES: usize = 24;
const USER_BASE_SCORE: u32 = 30_000;
const USER_FREQUENCY_STEP: u32 = 200;

#[derive(Debug, Clone, Copy, Eq, PartialEq)]
pub enum DictionaryKind {
    System,
    Name,
    Place,
    Station,
    Translation,
    User,
    Generated,
}

#[derive(Debug, Clone, Copy, Eq, PartialEq)]
pub enum CandidateMatch {
    Exact,
    Prefix,
    Middle,
    Generated,
}

#[derive(Debug, Clone, Eq, PartialEq)]
pub struct DictionaryCandidate {
    pub text: String,
    pub score: u32,
    pub kind: DictionaryKind,
    pub match_kind: CandidateMatch,
}

#[derive(Default)]
struct StaticDictionaries {
    frequent: HashMap<String, Vec<DictionaryCandidate>>,
    system: HashMap<String, Vec<DictionaryCandidate>>,
    katakana: HashMap<String, Vec<DictionaryCandidate>>,
    names: HashMap<String, Vec<DictionaryCandidate>>,
    places: HashMap<String, Vec<DictionaryCandidate>>,
    translations: HashMap<String, Vec<DictionaryCandidate>>,
    predictive: PredictiveIndex,
    specialized_predictive: PredictiveIndex,
    translation_predictive: PredictiveIndex,
}

#[derive(Default)]
struct PredictiveIndex {
    entries: Vec<ReadingEntry>,
    prefix_unigrams: HashMap<char, Vec<usize>>,
    unigrams: HashMap<char, Vec<usize>>,
    bigrams: HashMap<String, Vec<usize>>,
    surfaces: SurfaceIndex,
}

struct ReadingEntry {
    reading: String,
    candidates: Vec<DictionaryCandidate>,
}

#[derive(Default)]
struct SurfaceIndex {
    entries: Vec<DictionaryCandidate>,
    characters: HashMap<char, Vec<usize>>,
}

#[derive(Default)]
struct UserDictionary {
    entries: HashMap<String, HashMap<String, u32>>,
    predictive: PredictiveIndex,
}

enum NonKanaPredictiveMode {
    SurfaceText,
    ReadingKey,
}

static STATIC_DICTIONARIES: OnceLock<StaticDictionaries> = OnceLock::new();
static USER_DICTIONARY: OnceLock<RwLock<UserDictionary>> = OnceLock::new();
static USER_WAL_SENDER: OnceLock<Option<Sender<(String, String)>>> = OnceLock::new();

pub fn system_candidates(reading: &str) -> Vec<DictionaryCandidate> {
    let reading = normalize_kana_reading(reading);
    let dictionaries = static_dictionaries();
    let mut result = user_candidates(&reading);
    result.extend(lookup(&dictionaries.frequent, &reading));
    result.extend(lookup(&dictionaries.system, &reading));
    result.extend(lookup(&dictionaries.katakana, &reading));
    result
}

pub fn predictive_candidates(reading: &str) -> Vec<DictionaryCandidate> {
    let reading = normalize_kana_reading(reading);
    if reading.is_empty() {
        return Vec::new();
    }

    let mut result = user_predictive_candidates(&reading);
    if !reading.chars().any(|character| character.is_ascii()) {
        result.extend(predictive_candidates_from_index(
            &static_dictionaries().predictive,
            &reading,
            NonKanaPredictiveMode::SurfaceText,
        ));
    }
    sort_by_match_then_score(&mut result);
    result.truncate(MAX_PREDICTIVE_CANDIDATES);
    result
}

pub fn concatenated_candidates(reading: &str) -> Vec<DictionaryCandidate> {
    let reading = normalize_kana_reading(reading);
    let character_count = reading.chars().count();
    if !(4..=16).contains(&character_count) || reading.chars().any(|character| character.is_ascii())
    {
        return Vec::new();
    }

    let mut boundaries = reading
        .char_indices()
        .map(|(index, _)| index)
        .collect::<Vec<_>>();
    boundaries.push(reading.len());
    let mut result = Vec::new();
    build_concatenations(
        static_dictionaries(),
        &reading,
        &boundaries,
        0,
        &mut Vec::new(),
        &mut result,
    );
    sort_and_deduplicate(&mut result);
    result.truncate(MAX_CONCATENATED_CANDIDATES);
    result
}

pub fn warm_up() {
    let _ = static_dictionaries();
    let _ = user_dictionary();
}

pub fn specialized_candidates(reading: &str) -> Vec<DictionaryCandidate> {
    let dictionaries = static_dictionaries();
    let reading = normalize_kana_reading(reading);
    let mut result = lookup(&dictionaries.names, &reading);
    result.extend(lookup(&dictionaries.places, &reading));
    result
}

pub fn specialized_predictive_candidates(reading: &str) -> Vec<DictionaryCandidate> {
    let reading = normalize_kana_reading(reading);
    if reading.is_empty() || reading.chars().any(|character| character.is_ascii()) {
        return Vec::new();
    }

    predictive_candidates_from_index(
        &static_dictionaries().specialized_predictive,
        &reading,
        NonKanaPredictiveMode::SurfaceText,
    )
}

pub fn translation_candidates(raw_query: &str, kana_query: &str) -> Vec<DictionaryCandidate> {
    let dictionaries = static_dictionaries();
    let normalized_raw = normalize_translation_key(raw_query);
    let mut result = lookup(&dictionaries.translations, &normalized_raw);
    let normalized_kana = normalize_translation_key(kana_query);
    if normalized_kana != normalized_raw {
        result.extend(lookup(&dictionaries.translations, &normalized_kana));
    }
    result
}

pub fn translation_predictive_candidates(
    raw_query: &str,
    kana_query: &str,
) -> Vec<DictionaryCandidate> {
    let dictionaries = static_dictionaries();
    let normalized_raw = normalize_translation_key(raw_query);
    if normalized_raw.is_empty() || normalized_raw.chars().any(|character| character.is_ascii()) {
        return Vec::new();
    }

    let mut result = predictive_candidates_from_index(
        &dictionaries.translation_predictive,
        &normalized_raw,
        NonKanaPredictiveMode::ReadingKey,
    );

    let normalized_kana = normalize_translation_key(kana_query);
    if normalized_kana != normalized_raw
        && !normalized_kana.is_empty()
        && !normalized_kana
            .chars()
            .any(|character| character.is_ascii())
    {
        result.extend(predictive_candidates_from_index(
            &dictionaries.translation_predictive,
            &normalized_kana,
            NonKanaPredictiveMode::ReadingKey,
        ));
    }
    sort_by_match_then_score(&mut result);
    result.truncate(MAX_PREDICTIVE_CANDIDATES);
    result
}

pub fn record_selection(reading: &str, text: &str) {
    let reading = normalize_reading(&sanitize_field(reading));
    let text = sanitize_field(text);
    if reading.is_empty() || text.is_empty() {
        return;
    }

    increment_user_frequency(&reading, &text);

    if let Some(sender) = user_wal_sender() {
        let _ = sender.send((reading, text));
    }
}

fn increment_user_frequency(reading: &str, text: &str) -> u32 {
    let mut dictionary = user_dictionary()
        .write()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    let updated_frequency = {
        let frequency = dictionary
            .entries
            .entry(reading.to_string())
            .or_default()
            .entry(text.to_string())
            .or_default();
        *frequency = frequency.saturating_add(1);
        *frequency
    };
    rebuild_user_predictive_index(&mut dictionary);
    updated_frequency
}

pub fn record_legacy_frequency(value: &str) {
    if let Some((reading, text)) = value.split_once('\t') {
        record_selection(reading, text);
    } else {
        record_selection(value, value);
    }
}

fn static_dictionaries() -> &'static StaticDictionaries {
    STATIC_DICTIONARIES.get_or_init(build_static_dictionaries)
}

fn build_static_dictionaries() -> StaticDictionaries {
    let frequent = parse_tsv(FREQUENT_TSV, DictionaryKind::System, false);
    let system = parse_tsv_bundle(
        &[SYSTEM_TSV, SYSTEM_SUPPLEMENT_TSV],
        DictionaryKind::System,
        false,
    );
    let katakana = parse_tsv(KATAKANA_TSV, DictionaryKind::System, false);
    let names = parse_tsv_bundle(
        &[NAME_TSV, NAME_SUPPLEMENT_TSV],
        DictionaryKind::Name,
        false,
    );
    let places = parse_tsv_bundle(
        &[PLACE_TSV, PLACE_SUPPLEMENT_TSV],
        DictionaryKind::Place,
        true,
    );
    let translations = parse_tsv_bundle(
        &[TRANSLATION_TSV, TRANSLATION_SUPPLEMENT_TSV],
        DictionaryKind::Translation,
        false,
    );
    let predictive = build_predictive_index(
        &[&frequent, &system, &katakana],
        &[&frequent, &system, &katakana, &names, &places],
    );
    let specialized_predictive = build_predictive_index(&[&names, &places], &[&names, &places]);
    let translation_predictive = build_predictive_index(&[&translations], &[&translations]);
    StaticDictionaries {
        frequent,
        system,
        katakana,
        names,
        places,
        translations,
        predictive,
        specialized_predictive,
        translation_predictive,
    }
}

fn user_dictionary() -> &'static RwLock<UserDictionary> {
    USER_DICTIONARY.get_or_init(|| RwLock::new(load_user_wal()))
}

fn user_wal_sender() -> Option<&'static Sender<(String, String)>> {
    USER_WAL_SENDER
        .get_or_init(|| {
            let (sender, receiver) = mpsc::channel::<(String, String)>();
            let worker = thread::Builder::new()
                .name("keyro-user-dictionary-wal".to_string())
                .spawn(move || {
                    while let Ok((reading, text)) = receiver.recv() {
                        append_user_wal(&reading, &text);
                    }
                });
            worker.ok().map(|_| sender)
        })
        .as_ref()
}

fn lookup(
    dictionary: &HashMap<String, Vec<DictionaryCandidate>>,
    key: &str,
) -> Vec<DictionaryCandidate> {
    dictionary.get(key).cloned().unwrap_or_default()
}

fn parse_tsv(
    contents: &str,
    default_kind: DictionaryKind,
    has_kind_column: bool,
) -> HashMap<String, Vec<DictionaryCandidate>> {
    parse_tsv_bundle(&[contents], default_kind, has_kind_column)
}

fn parse_tsv_bundle(
    sources: &[&str],
    default_kind: DictionaryKind,
    has_kind_column: bool,
) -> HashMap<String, Vec<DictionaryCandidate>> {
    let mut dictionary: HashMap<String, Vec<DictionaryCandidate>> = HashMap::new();
    for contents in sources {
        for line in contents.lines() {
            let line = line.trim();
            if line.is_empty() || line.starts_with('#') {
                continue;
            }

            let columns = line.split('\t').collect::<Vec<_>>();
            if columns.len() < 3 {
                continue;
            }

            let kind = if has_kind_column && columns.len() >= 4 {
                match columns[3] {
                    "station" => DictionaryKind::Station,
                    "place" => DictionaryKind::Place,
                    _ => default_kind,
                }
            } else {
                default_kind
            };
            let score = columns[2].parse::<u32>().unwrap_or(100);
            let source = columns[0].trim();
            let target = columns[1].trim();
            dictionary
                .entry(normalize_dictionary_key(source, kind))
                .or_default()
                .push(DictionaryCandidate {
                    text: target.to_string(),
                    score,
                    kind,
                    match_kind: CandidateMatch::Exact,
                });

            if kind == DictionaryKind::Translation {
                let reverse_key = normalize_translation_key(target);
                let reverse_text = translation_reverse_text(source);
                if !reverse_key.is_empty() && !reverse_text.is_empty() && reverse_text != target {
                    dictionary
                        .entry(reverse_key)
                        .or_default()
                        .push(DictionaryCandidate {
                            text: reverse_text,
                            score,
                            kind,
                            match_kind: CandidateMatch::Exact,
                        });
                }
            }
        }
    }

    for candidates in dictionary.values_mut() {
        candidates.sort_by(|left, right| right.score.cmp(&left.score));
    }
    dictionary
}

fn user_candidates(reading: &str) -> Vec<DictionaryCandidate> {
    let dictionary = user_dictionary()
        .read()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    let mut result = dictionary
        .entries
        .get(reading)
        .map(|entries| {
            entries
                .iter()
                .map(|(text, frequency)| DictionaryCandidate {
                    text: text.clone(),
                    score: user_score(*frequency),
                    kind: DictionaryKind::User,
                    match_kind: CandidateMatch::Exact,
                })
                .collect::<Vec<_>>()
        })
        .unwrap_or_default();
    result.sort_by(|left, right| right.score.cmp(&left.score));
    result
}

fn user_predictive_candidates(query: &str) -> Vec<DictionaryCandidate> {
    let dictionary = user_dictionary()
        .read()
        .unwrap_or_else(|poisoned| poisoned.into_inner());
    let mut result = predictive_candidates_from_index(
        &dictionary.predictive,
        query,
        NonKanaPredictiveMode::SurfaceText,
    );
    result.truncate(MAX_USER_PREDICTIVE_CANDIDATES);
    result
}

fn user_score(frequency: u32) -> u32 {
    USER_BASE_SCORE.saturating_add(frequency.saturating_mul(USER_FREQUENCY_STEP))
}

fn rebuild_user_predictive_index(dictionary: &mut UserDictionary) {
    let candidates = dictionary
        .entries
        .iter()
        .map(|(reading, entries)| {
            let candidates = entries
                .iter()
                .map(|(text, frequency)| DictionaryCandidate {
                    text: text.clone(),
                    score: user_score(*frequency),
                    kind: DictionaryKind::User,
                    match_kind: CandidateMatch::Exact,
                })
                .collect::<Vec<_>>();
            (reading.clone(), candidates)
        })
        .collect::<HashMap<_, _>>();
    dictionary.predictive = build_predictive_index(&[&candidates], &[&candidates]);
}

fn load_user_wal() -> UserDictionary {
    let mut dictionary = UserDictionary::default();
    let Ok(contents) = fs::read_to_string(user_wal_path()) else {
        return dictionary;
    };

    for line in contents.lines() {
        let mut columns = line.split('\t');
        let (Some(reading), Some(text), Some(delta)) =
            (columns.next(), columns.next(), columns.next())
        else {
            continue;
        };
        let delta = delta.parse::<u32>().unwrap_or(0);
        let frequency = dictionary
            .entries
            .entry(reading.to_string())
            .or_default()
            .entry(text.to_string())
            .or_default();
        *frequency = frequency.saturating_add(delta);
    }
    rebuild_user_predictive_index(&mut dictionary);
    dictionary
}

fn append_user_wal(reading: &str, text: &str) {
    let path = user_wal_path();
    if let Some(parent) = path.parent() {
        if fs::create_dir_all(parent).is_err() {
            return;
        }
    }
    if let Ok(mut file) = OpenOptions::new().create(true).append(true).open(path) {
        let _ = writeln!(file, "{reading}\t{text}\t1");
    }
}

fn user_wal_path() -> PathBuf {
    let base = env::var_os("PROGRAMDATA")
        .map(PathBuf::from)
        .unwrap_or_else(env::temp_dir);
    base.join("KeyroIME").join("user_dictionary.wal")
}

fn sanitize_field(value: &str) -> String {
    value
        .chars()
        .filter(|character| *character != '\t' && *character != '\r' && *character != '\n')
        .collect::<String>()
        .trim()
        .to_string()
}

fn normalize_reading(reading: &str) -> String {
    let converted = if RomajiConverter::is_valid_romaji(reading) {
        RomajiConverter::new().convert(reading)
    } else {
        reading.to_string()
    };
    normalize_kana_reading(&converted)
}

fn normalize_dictionary_key(value: &str, kind: DictionaryKind) -> String {
    if kind == DictionaryKind::Translation {
        normalize_translation_key(value)
    } else {
        normalize_kana_reading(value)
    }
}

fn normalize_translation_key(value: &str) -> String {
    let trimmed = value.trim();
    if trimmed.is_ascii() {
        trimmed.to_ascii_lowercase()
    } else {
        normalize_kana_reading(trimmed)
    }
}

fn normalize_kana_reading(reading: &str) -> String {
    reading
        .trim()
        .chars()
        .map(|character| {
            if ('ァ'..='ヶ').contains(&character) {
                char::from_u32(character as u32 - 0x60).unwrap_or(character)
            } else {
                character
            }
        })
        .collect()
}

fn translation_reverse_text(source: &str) -> String {
    let trimmed = source.trim();
    let normalized = normalize_kana_reading(trimmed);
    if is_kana_reading(&normalized) {
        hiragana_to_katakana(&normalized)
    } else {
        trimmed.to_string()
    }
}

fn is_kana_reading(value: &str) -> bool {
    !value.is_empty()
        && value
            .chars()
            .all(|character| ('ぁ'..='ゖ').contains(&character) || character == 'ー')
}

fn hiragana_to_katakana(value: &str) -> String {
    value
        .chars()
        .map(|character| {
            if ('ぁ'..='ゖ').contains(&character) {
                char::from_u32(character as u32 + 0x60).unwrap_or(character)
            } else {
                character
            }
        })
        .collect()
}

fn build_predictive_index(
    reading_dictionaries: &[&HashMap<String, Vec<DictionaryCandidate>>],
    surface_dictionaries: &[&HashMap<String, Vec<DictionaryCandidate>>],
) -> PredictiveIndex {
    let mut grouped: HashMap<String, Vec<DictionaryCandidate>> = HashMap::new();
    for dictionary in reading_dictionaries {
        for (reading, candidates) in dictionary.iter() {
            grouped
                .entry(reading.clone())
                .or_default()
                .extend(candidates.iter().cloned());
        }
    }

    let mut entries = grouped
        .into_iter()
        .map(|(reading, mut candidates)| {
            sort_and_deduplicate(&mut candidates);
            ReadingEntry {
                reading,
                candidates,
            }
        })
        .collect::<Vec<_>>();
    entries.sort_by(|left, right| left.reading.cmp(&right.reading));

    let mut prefix_unigrams: HashMap<char, Vec<usize>> = HashMap::new();
    let mut unigrams: HashMap<char, Vec<usize>> = HashMap::new();
    let mut bigrams: HashMap<String, Vec<usize>> = HashMap::new();
    for (entry_index, entry) in entries.iter().enumerate() {
        let characters = entry.reading.chars().collect::<Vec<_>>();
        if let Some(first) = characters.first() {
            prefix_unigrams.entry(*first).or_default().push(entry_index);
        }
        let mut seen = HashSet::new();
        for character in &characters {
            if seen.insert(character.to_string()) {
                unigrams.entry(*character).or_default().push(entry_index);
            }
        }
        for pair in characters.windows(2) {
            let bigram = pair.iter().collect::<String>();
            if seen.insert(bigram.clone()) {
                bigrams.entry(bigram).or_default().push(entry_index);
            }
        }
    }

    sort_and_limit_postings(&entries, &mut prefix_unigrams);
    sort_and_limit_postings(&entries, &mut unigrams);
    sort_and_limit_postings(&entries, &mut bigrams);
    let surfaces = build_surface_index(surface_dictionaries);
    PredictiveIndex {
        entries,
        prefix_unigrams,
        unigrams,
        bigrams,
        surfaces,
    }
}

fn sort_and_limit_postings<K: Eq + std::hash::Hash>(
    entries: &[ReadingEntry],
    postings: &mut HashMap<K, Vec<usize>>,
) {
    for indices in postings.values_mut() {
        indices.sort_by(|left, right| {
            reading_entry_score(&entries[*right])
                .cmp(&reading_entry_score(&entries[*left]))
                .then_with(|| entries[*left].reading.cmp(&entries[*right].reading))
        });
        indices.truncate(MAX_FRAGMENT_POSTINGS);
    }
}

fn reading_entry_score(entry: &ReadingEntry) -> u32 {
    entry
        .candidates
        .iter()
        .map(|candidate| candidate.score)
        .max()
        .unwrap_or_default()
}

fn build_surface_index(
    dictionaries: &[&HashMap<String, Vec<DictionaryCandidate>>],
) -> SurfaceIndex {
    let mut best_by_text: HashMap<String, DictionaryCandidate> = HashMap::new();
    for dictionary in dictionaries {
        for candidates in dictionary.values() {
            for candidate in candidates {
                let replace = best_by_text
                    .get(&candidate.text)
                    .map(|current| candidate.score > current.score)
                    .unwrap_or(true);
                if replace {
                    best_by_text.insert(candidate.text.clone(), candidate.clone());
                }
            }
        }
    }

    let mut entries = best_by_text.into_values().collect::<Vec<_>>();
    entries.sort_by(|left, right| {
        right
            .score
            .cmp(&left.score)
            .then_with(|| left.text.cmp(&right.text))
    });
    let mut characters: HashMap<char, Vec<usize>> = HashMap::new();
    for (entry_index, entry) in entries.iter().enumerate() {
        let mut seen = HashSet::new();
        for character in entry.text.chars() {
            if seen.insert(character) {
                characters.entry(character).or_default().push(entry_index);
            }
        }
    }
    for indices in characters.values_mut() {
        indices.truncate(MAX_FRAGMENT_POSTINGS);
    }
    SurfaceIndex {
        entries,
        characters,
    }
}

fn predictive_candidates_from_index(
    index: &PredictiveIndex,
    query: &str,
    non_kana_mode: NonKanaPredictiveMode,
) -> Vec<DictionaryCandidate> {
    if query.is_empty() {
        return Vec::new();
    }

    if !query.chars().all(is_kana_character) {
        return match non_kana_mode {
            NonKanaPredictiveMode::SurfaceText => {
                predictive_surface_candidates(&index.surfaces, query)
            }
            NonKanaPredictiveMode::ReadingKey => predictive_reading_candidates(index, query),
        };
    }

    predictive_reading_candidates(index, query)
}

fn predictive_reading_candidates(index: &PredictiveIndex, query: &str) -> Vec<DictionaryCandidate> {
    let mut result = Vec::new();
    let query_length = query.chars().count();
    let prefix_entries = if query_length == 1 {
        query
            .chars()
            .next()
            .and_then(|character| index.prefix_unigrams.get(&character))
            .cloned()
            .unwrap_or_default()
    } else {
        first_bigram(query)
            .and_then(|bigram| index.bigrams.get(&bigram))
            .map(|entry_indices| {
                entry_indices
                    .iter()
                    .copied()
                    .filter(|entry_index| index.entries[*entry_index].reading.starts_with(query))
                    .collect::<Vec<_>>()
            })
            .unwrap_or_default()
    };

    for entry_index in prefix_entries {
        let entry = &index.entries[entry_index];
        if entry.reading == query {
            continue;
        }
        let distance = entry.reading.chars().count().saturating_sub(query_length);
        result.extend(
            entry
                .candidates
                .iter()
                .take(3)
                .cloned()
                .map(|mut candidate| {
                    candidate.score = predictive_score(candidate.score, distance, true);
                    candidate.match_kind = CandidateMatch::Prefix;
                    candidate
                }),
        );
        if result.len() >= MAX_PREFIX_PREDICTIVE_CANDIDATES {
            break;
        }
    }
    result.truncate(MAX_PREFIX_PREDICTIVE_CANDIDATES);

    let middle_entries = if query_length == 1 {
        query
            .chars()
            .next()
            .and_then(|character| index.unigrams.get(&character))
    } else {
        first_bigram(query).and_then(|bigram| index.bigrams.get(&bigram))
    };
    if let Some(entry_indices) = middle_entries {
        for &entry_index in entry_indices {
            let entry = &index.entries[entry_index];
            if entry.reading.starts_with(query) || !entry.reading.contains(query) {
                continue;
            }
            let distance = entry.reading.chars().count().saturating_sub(query_length);
            result.extend(
                entry
                    .candidates
                    .iter()
                    .take(3)
                    .cloned()
                    .map(|mut candidate| {
                        candidate.score = predictive_score(candidate.score, distance, false);
                        candidate.match_kind = CandidateMatch::Middle;
                        candidate
                    }),
            );
            if result.len() >= MAX_PREDICTIVE_CANDIDATES * 2 {
                break;
            }
        }
    }

    sort_by_match_then_score(&mut result);
    result.truncate(MAX_PREDICTIVE_CANDIDATES);
    result
}

fn predictive_surface_candidates(index: &SurfaceIndex, query: &str) -> Vec<DictionaryCandidate> {
    let Some(first_character) = query.chars().next() else {
        return Vec::new();
    };
    let Some(entry_indices) = index.characters.get(&first_character) else {
        return Vec::new();
    };

    let query_length = query.chars().count();
    let mut prefix = Vec::new();
    let mut middle = Vec::new();
    for &entry_index in entry_indices {
        let entry = &index.entries[entry_index];
        if entry.text == query {
            let mut exact = entry.clone();
            exact.match_kind = CandidateMatch::Exact;
            prefix.push(exact);
        } else if entry.text.starts_with(query) {
            let mut candidate = entry.clone();
            let distance = candidate.text.chars().count().saturating_sub(query_length);
            candidate.score = predictive_score(candidate.score, distance, true);
            candidate.match_kind = CandidateMatch::Prefix;
            prefix.push(candidate);
        } else if entry.text.contains(query) {
            let mut candidate = entry.clone();
            let distance = candidate.text.chars().count().saturating_sub(query_length);
            candidate.score = predictive_score(candidate.score, distance, false);
            candidate.match_kind = CandidateMatch::Middle;
            middle.push(candidate);
        }
    }
    sort_and_deduplicate(&mut prefix);
    sort_and_deduplicate(&mut middle);
    prefix.truncate(MAX_PREFIX_PREDICTIVE_CANDIDATES);
    prefix.extend(middle);
    sort_by_match_then_score(&mut prefix);
    prefix.truncate(MAX_PREDICTIVE_CANDIDATES);
    prefix
}

fn predictive_score(original_score: u32, distance: usize, is_prefix: bool) -> u32 {
    let tier = if is_prefix { 1_600_u32 } else { 1_200_u32 };
    tier.saturating_add(original_score.min(4_000) / 4)
        .saturating_sub((distance as u32).saturating_mul(20).min(300))
}

fn is_kana_character(character: char) -> bool {
    ('ぁ'..='ゖ').contains(&character) || character == 'ー'
}

fn first_bigram(reading: &str) -> Option<String> {
    let bigram = reading.chars().take(2).collect::<String>();
    (bigram.chars().count() == 2).then_some(bigram)
}

fn exact_segment_candidates(
    dictionaries: &StaticDictionaries,
    reading: &str,
) -> Vec<DictionaryCandidate> {
    let mut candidates = user_candidates(reading);
    candidates.extend(lookup(&dictionaries.frequent, reading));
    candidates.extend(lookup(&dictionaries.system, reading));
    candidates.extend(lookup(&dictionaries.katakana, reading));
    candidates.extend(lookup(&dictionaries.names, reading));
    candidates.extend(lookup(&dictionaries.places, reading));
    sort_and_deduplicate(&mut candidates);
    candidates.truncate(2);
    candidates
}

fn build_concatenations(
    dictionaries: &StaticDictionaries,
    reading: &str,
    boundaries: &[usize],
    start_character: usize,
    parts: &mut Vec<(String, usize)>,
    result: &mut Vec<DictionaryCandidate>,
) {
    if parts.len() >= MAX_CONCATENATION_PARTS {
        return;
    }
    let character_count = boundaries.len() - 1;
    for end_character in (start_character + 2)..=character_count {
        let remaining = character_count - end_character;
        if remaining == 1 || (remaining > 0 && parts.len() + 1 >= MAX_CONCATENATION_PARTS) {
            continue;
        }
        let segment = &reading[boundaries[start_character]..boundaries[end_character]];
        let candidates = exact_segment_candidates(dictionaries, segment);
        if candidates.is_empty() {
            continue;
        }
        let variant_limit = if parts.is_empty() { 2 } else { 1 };
        for (variant, candidate) in candidates.into_iter().take(variant_limit).enumerate() {
            parts.push((candidate.text, variant));
            if end_character == character_count {
                if parts.len() >= 2 {
                    let text = parts
                        .iter()
                        .map(|(surface, _)| surface.as_str())
                        .collect::<String>();
                    if text != reading {
                        let variant_penalty =
                            parts.iter().map(|(_, index)| *index as u32).sum::<u32>() * 25;
                        let part_penalty = (parts.len().saturating_sub(2) as u32) * 100;
                        result.push(DictionaryCandidate {
                            text,
                            score: 950_u32.saturating_sub(variant_penalty + part_penalty),
                            kind: DictionaryKind::Generated,
                            match_kind: CandidateMatch::Generated,
                        });
                    }
                }
            } else {
                build_concatenations(
                    dictionaries,
                    reading,
                    boundaries,
                    end_character,
                    parts,
                    result,
                );
            }
            parts.pop();
            if result.len() >= MAX_CONCATENATED_CANDIDATES * 4 {
                return;
            }
        }
    }
}

fn sort_and_deduplicate(candidates: &mut Vec<DictionaryCandidate>) {
    candidates.sort_by(|left, right| {
        right
            .score
            .cmp(&left.score)
            .then_with(|| left.text.cmp(&right.text))
    });
    let mut seen = HashSet::new();
    candidates
        .retain(|candidate| !candidate.text.is_empty() && seen.insert(candidate.text.clone()));
}

fn sort_by_match_then_score(candidates: &mut Vec<DictionaryCandidate>) {
    candidates.sort_by(|left, right| {
        match_priority(right.match_kind)
            .cmp(&match_priority(left.match_kind))
            .then_with(|| right.score.cmp(&left.score))
            .then_with(|| left.text.cmp(&right.text))
    });
    let mut seen = HashSet::new();
    candidates.retain(|candidate| seen.insert(candidate.text.clone()));
}

fn match_priority(match_kind: CandidateMatch) -> u8 {
    match match_kind {
        CandidateMatch::Exact => 4,
        CandidateMatch::Prefix => 3,
        CandidateMatch::Middle => 2,
        CandidateMatch::Generated => 1,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn static_assets_cover_all_dictionary_classes() {
        assert!(system_candidates("こうかん")
            .iter()
            .any(|item| item.text == "交換"));
        assert!(specialized_candidates("おおつき")
            .iter()
            .any(|item| item.text == "大月駅"));
        assert!(specialized_candidates("はると")
            .iter()
            .any(|item| item.text == "陽翔"));
        assert!(translation_candidates("check", "ちぇっく")
            .iter()
            .any(|item| item.text == "確認する"));
        assert!(translation_candidates("nihon", "にほん")
            .iter()
            .any(|item| item.text == "Japan"));
    }

    #[test]
    fn normalizes_dynamic_romaji_reading() {
        assert_eq!(normalize_reading("koukan"), "こうかん");
        assert_eq!(normalize_reading("にほん"), "にほん");
    }

    #[test]
    fn frequent_dictionary_precedes_regular_system_entries() {
        let candidates = system_candidates("こうかん");
        assert!(!candidates.is_empty());
        assert!(candidates.iter().any(|item| item.text == "交換"));
        let best_exchange_score = candidates
            .iter()
            .filter(|item| item.text == "交換")
            .map(|item| item.score)
            .max()
            .unwrap_or_default();
        assert!(best_exchange_score >= 2_300);
    }

    #[test]
    fn dictionary_profile_exceeds_development_sample_floor() {
        let count = |contents: &str| {
            contents
                .lines()
                .filter(|line| !line.is_empty() && !line.starts_with('#'))
                .count()
        };
        assert!(count(SYSTEM_TSV) + count(SYSTEM_SUPPLEMENT_TSV) >= 1_000);
        assert!(count(FREQUENT_TSV) >= 100);
        assert!(count(NAME_TSV) + count(NAME_SUPPLEMENT_TSV) >= 100);
        assert!(count(PLACE_TSV) + count(PLACE_SUPPLEMENT_TSV) >= 100);
        assert!(count(TRANSLATION_TSV) + count(TRANSLATION_SUPPLEMENT_TSV) >= 100);
        assert!(count(KATAKANA_TSV) >= 100);
    }

    #[test]
    fn curated_supplements_are_loaded_with_static_assets() {
        assert!(system_candidates("きーろ")
            .iter()
            .any(|item| item.text == "KeyroIME"));
        assert!(specialized_candidates("しんじゅく")
            .iter()
            .any(|item| item.text == "新宿"));
        assert!(specialized_candidates("れん")
            .iter()
            .any(|item| item.text == "蓮"));
        assert!(translation_candidates("translation", "translation")
            .iter()
            .any(|item| item.text == "訳"));
    }

    #[test]
    fn dynamic_frequency_increases_user_candidate_score() {
        let reading = "どうてきしけんせんよう";
        let text = "動的試験専用";
        let first = increment_user_frequency(reading, text);
        let second = increment_user_frequency(reading, text);
        assert_eq!(second, first + 1);

        let candidate = user_candidates(reading)
            .into_iter()
            .find(|item| item.text == text)
            .expect("dynamic candidate should be returned");
        assert_eq!(candidate.kind, DictionaryKind::User);
        assert_eq!(
            candidate.score,
            USER_BASE_SCORE + second * USER_FREQUENCY_STEP
        );
    }

    #[test]
    fn user_candidates_participate_in_reading_and_surface_prediction() {
        let reading = "ゆーざーれんそうしけん";
        let text = "利用者連想試験";
        increment_user_frequency(reading, text);

        let reading_prediction = predictive_candidates("ゆーざーれん")
            .into_iter()
            .find(|candidate| candidate.text == text)
            .expect("user reading prediction should be returned");
        assert_eq!(reading_prediction.kind, DictionaryKind::User);
        assert_eq!(reading_prediction.match_kind, CandidateMatch::Prefix);

        let surface_prediction = predictive_candidates("利用者")
            .into_iter()
            .find(|candidate| candidate.text == text)
            .expect("user surface prediction should be returned");
        assert_eq!(surface_prediction.kind, DictionaryKind::User);
        assert_eq!(surface_prediction.match_kind, CandidateMatch::Prefix);
    }

    #[test]
    fn katakana_assets_are_recalled_from_hiragana_readings() {
        let install = system_candidates("いんすとーる");
        assert!(install.iter().any(|item| item.text == "インストール"));

        let legacy_katakana_key = system_candidates("あぷりけーしょん");
        assert!(legacy_katakana_key
            .iter()
            .any(|item| item.text == "アプリケーション"));

        let direct_katakana_key = system_candidates("ゼロトラスト");
        assert!(direct_katakana_key
            .iter()
            .any(|item| item.text == "ゼロトラスト"));

        let terminology_expansion = system_candidates("あくせしびりてぃー");
        assert!(terminology_expansion
            .iter()
            .any(|item| item.text == "アクセシビリティー"));
    }

    #[test]
    fn local_dictionary_corpus_import_is_loaded() {
        let system_term = system_candidates("りんぎしょ");
        assert!(system_term.iter().any(|item| item.text == "稟議書"));

        let katakana_term = system_candidates("わんおぺ");
        assert!(katakana_term.iter().any(|item| item.text == "ワンオペ"));

        let technical_term = system_candidates("ばっくろぐ");
        assert!(technical_term.iter().any(|item| item.text == "バックログ"));
    }

    #[test]
    fn one_kana_and_kanji_start_predictive_lookup() {
        let one_kana = predictive_candidates("つ");
        assert!(!one_kana.is_empty());
        assert!(one_kana.iter().any(|candidate| candidate.text == "使う"));
        assert!(one_kana
            .iter()
            .any(|candidate| candidate.match_kind == CandidateMatch::Middle));

        let one_kanji = predictive_candidates("承");
        assert!(one_kanji.iter().any(|candidate| candidate.text == "承知"));
        assert!(one_kanji
            .iter()
            .any(|candidate| candidate.text == "承知しました"));
    }

    #[test]
    fn q_and_v_sources_support_short_predictive_lookup() {
        let schedule = translation_predictive_candidates("す", "す");
        assert!(schedule
            .iter()
            .any(|candidate| candidate.text == "schedule"));

        let japan = translation_predictive_candidates("日", "日");
        assert!(japan.iter().any(|candidate| candidate.text == "Japan"));

        let place_by_reading = specialized_predictive_candidates("し");
        assert!(place_by_reading
            .iter()
            .any(|candidate| candidate.text == "新宿"));

        let place_by_surface = specialized_predictive_candidates("新");
        assert!(place_by_surface
            .iter()
            .any(|candidate| candidate.text == "新宿"));
    }

    #[test]
    fn static_predictive_lookup_prioritizes_prefix_before_middle_matches() {
        let static_predictions = |query| {
            predictive_candidates_from_index(
                &static_dictionaries().predictive,
                query,
                NonKanaPredictiveMode::SurfaceText,
            )
        };
        let candidates = static_predictions("つか");
        let position = |text: &str| {
            candidates
                .iter()
                .position(|candidate| candidate.text == text)
                .expect("predictive candidate should exist")
        };
        assert!(position("使う") < position("お疲れ様"));
        assert!(position("遣う") < position("お疲れ様です"));
        assert!(position("お疲れ様") < 15);
        assert!(position("お疲れ様でした") < 15);
        assert!(candidates.iter().any(|item| item.text == "使い"));
        assert!(candidates.iter().any(|item| item.text == "疲れ"));

        let acknowledgement = static_predictions("しょうち");
        assert!(acknowledgement
            .iter()
            .any(|item| item.text == "承知しました"));
        assert!(acknowledgement
            .iter()
            .any(|item| item.text == "承知いたしました"));
    }

    #[test]
    fn concatenation_combines_multiple_exact_dictionary_fragments() {
        let candidates = concatenated_candidates("しようしょ");
        assert!(candidates.iter().any(|item| item.text == "仕様書"));
        assert!(candidates.iter().any(|item| item.text == "使用書"));
        assert!(!candidates.iter().any(|item| item.text.ends_with('諸')));
        assert!(candidates
            .iter()
            .all(|item| item.kind == DictionaryKind::Generated));

        let proper_noun_fragments = concatenated_candidates("しんじゅくれん");
        assert!(proper_noun_fragments
            .iter()
            .any(|item| item.text == "新宿蓮"));
    }
}
