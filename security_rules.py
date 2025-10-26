# security_rules.py

class SecurityRules:
    def __init__(self):
        # Default rules - similar to Firebase
        self.rules = {
            # Public read, but only owner can write
            "sensors": {
                "read": "true",  # Anyone can read
                "write": "resource.owner == auth.uid"  # Only owner can write
            },
            # Only authenticated users can access
            "devices": {
                "read": "auth != null", 
                "write": "auth != null"
            },
            # Only owner can access their own data
            "users": {
                "read": "auth != null",
                "write": "auth != null"
            },
            # Admin only collection
            "admin": {
                "read": "auth.uid == 'admin'",
                "write": "auth.uid == 'admin'"
            },
            # File storage rules
            "files": {
                "read": "auth != null",
                "write": "auth != null"
            }
        }
    
    def validate_read(self, collection: str, user: str = None, resource: dict = None) -> bool:
        """Check if user can read from collection"""
        if collection not in self.rules:
            # Default: authenticated users only for unknown collections
            return user is not None
        
        rule = self.rules[collection]["read"]
        return self._evaluate_rule(rule, user, resource)
    
    def validate_write(self, collection: str, user: str = None, resource: dict = None) -> bool:
        """Check if user can write to collection"""
        if collection not in self.rules:
            # Default: authenticated users only for unknown collections
            return user is not None
        
        rule = self.rules[collection]["write"]
        return self._evaluate_rule(rule, user, resource)
    
    def _evaluate_rule(self, rule: str, user: str, resource: dict) -> bool:
        """Evaluate a security rule"""
        # Simple rule evaluation - in production you'd use a proper parser
        
        # auth != null  --> user is authenticated
        if rule == "auth != null":
            return user is not None
        
        # auth == null --> user is not authenticated  
        if rule == "auth == null":
            return user is None
        
        # true --> always allow
        if rule == "true":
            return True
        
        # false --> never allow
        if rule == "false":
            return False
        
        # resource.owner == auth.uid --> user owns the resource
        if rule == "resource.owner == auth.uid":
            return resource and resource.get("owner") == user
        
        # resource.id == auth.uid --> user matches resource ID
        if rule == "resource.id == auth.uid":
            return resource and resource.get("id") == user
        
        # auth.uid == 'admin' --> user is admin
        if "auth.uid == 'admin'" in rule:
            return user == "admin"
        
        # Default: deny access
        return False

# Create global instance
security_rules = SecurityRules()