use std::env;
use std::path::PathBuf;

fn main() {
    println!("cargo:rerun-if-env-changed=KEYROIME_DICTIONARY_ASSET_DIR");

    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR"));
    let asset_dir = env::var_os("KEYROIME_DICTIONARY_ASSET_DIR")
        .map(PathBuf::from)
        .unwrap_or_else(|| manifest_dir.join("assets"));
    let asset_dir = asset_dir.canonicalize().unwrap_or(asset_dir);
    let asset_dir = asset_dir.to_string_lossy().replace('\\', "/");

    for file_name in [
        "system.tsv",
        "frequent.tsv",
        "names.tsv",
        "places.tsv",
        "translations.tsv",
        "system_supplement.tsv",
        "names_supplement.tsv",
        "places_supplement.tsv",
        "translations_supplement.tsv",
        "katakana.tsv",
    ] {
        println!("cargo:rerun-if-changed={asset_dir}/{file_name}");
    }
    println!("cargo:rustc-env=KEYROIME_ASSET_DIR={asset_dir}");
}
