/// Tool to generate Argon2 password hashes for seed data
///
/// Usage: cargo run --bin generate_password_hash <password>
///
/// Example: cargo run --bin generate_password_hash demo123

use argon2::{
    password_hash::{rand_core::OsRng, PasswordHasher, SaltString},
    Argon2,
};

fn main() {
    let args: Vec<String> = std::env::args().collect();

    if args.len() != 2 {
        eprintln!("Usage: {} <password>", args[0]);
        std::process::exit(1);
    }

    let password = &args[1];

    // Generate salt
    let salt = SaltString::generate(&mut OsRng);

    // Hash password
    let argon2 = Argon2::default();
    let password_hash = argon2
        .hash_password(password.as_bytes(), &salt)
        .expect("Failed to hash password")
        .to_string();

    println!("Password: {}", password);
    println!("Hash: {}", password_hash);
}
