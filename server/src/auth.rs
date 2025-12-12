use crate::error::{Result, VoipError};
use crate::types::{Claims, UserId, OrgId, RoleId};
use argon2::{Argon2, PasswordHash, PasswordHasher, PasswordVerifier};
use argon2::password_hash::{SaltString, rand_core::OsRng};
use jsonwebtoken::{encode, decode, Header, EncodingKey, DecodingKey, Validation};
use chrono::Utc;
use uuid::Uuid;

/// Hash a password using Argon2id
pub fn hash_password(password: &str) -> Result<String> {
    let salt = SaltString::generate(&mut OsRng);
    let argon2 = Argon2::default();
    
    let password_hash = argon2
        .hash_password(password.as_bytes(), &salt)
        .map_err(|e| VoipError::Other(format!("Password hashing failed: {}", e)))?;
    
    Ok(password_hash.to_string())
}

/// Verify a password against a hash
pub fn verify_password(password: &str, hash: &str) -> Result<bool> {
    let parsed_hash = PasswordHash::new(hash)
        .map_err(|e| VoipError::Other(format!("Invalid password hash: {}", e)))?;
    
    let argon2 = Argon2::default();
    
    Ok(argon2.verify_password(password.as_bytes(), &parsed_hash).is_ok())
}

/// Create a JWT token
pub fn create_jwt(
    user_id: UserId,
    org_id: OrgId,
    roles: Vec<RoleId>,
    jwt_secret: &str,
    duration_hours: i64,
) -> Result<String> {
    let now = Utc::now();
    
    let claims = Claims {
        sub: user_id,
        org: org_id,
        roles,
        exp: (now + chrono::Duration::hours(duration_hours)).timestamp(),
        iat: now.timestamp(),
        jti: Uuid::new_v4().to_string(),
    };
    
    let token = encode(
        &Header::default(),
        &claims,
        &EncodingKey::from_secret(jwt_secret.as_bytes()),
    )?;
    
    Ok(token)
}

/// Verify and decode a JWT token
pub fn verify_jwt(token: &str, jwt_secret: &str) -> Result<Claims> {
    let token_data = decode::<Claims>(
        token,
        &DecodingKey::from_secret(jwt_secret.as_bytes()),
        &Validation::default(),
    )?;
    
    Ok(token_data.claims)
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_password_hash_and_verify() {
        let password = "test_password_123";
        
        let hash = hash_password(password).unwrap();
        assert_ne!(hash, password);
        
        assert!(verify_password(password, &hash).unwrap());
        assert!(!verify_password("wrong_password", &hash).unwrap());
    }
    
    #[test]
    fn test_jwt_create_and_verify() {
        let secret = "test_secret";
        let user_id = 1;
        let org_id = 1;
        let roles = vec![1, 2, 3];
        
        let token = create_jwt(user_id, org_id, roles.clone(), secret, 1).unwrap();
        assert!(!token.is_empty());
        
        let claims = verify_jwt(&token, secret).unwrap();
        assert_eq!(claims.sub, user_id);
        assert_eq!(claims.org, org_id);
        assert_eq!(claims.roles, roles);
    }
    
    #[test]
    fn test_jwt_invalid_secret() {
        let secret = "test_secret";
        let wrong_secret = "wrong_secret";
        
        let token = create_jwt(1, 1, vec![], secret, 1).unwrap();
        
        assert!(verify_jwt(&token, wrong_secret).is_err());
    }
}
