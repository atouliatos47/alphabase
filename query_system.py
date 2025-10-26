# query_system.py
import operator
import json
from typing import List, Dict, Any, Optional

class QueryParser:
    """Parse query parameters into query objects"""
    
    @staticmethod
    def parse_where_condition(condition: str) -> Dict[str, Any]:
        """Parse a where condition like 'temperature>25' or 'name==John'"""
        operators = ['>=', '<=', '!=', '==', '>', '<', '=']
        
        for op in operators:
            if op in condition:
                field, value = condition.split(op, 1)
                field = field.strip()
                
                # Try to convert value to appropriate type
                try:
                    if value.lower() == 'true':
                        value = True
                    elif value.lower() == 'false':
                        value = False
                    elif value.replace('.', '').isdigit():
                        value = float(value) if '.' in value else int(value)
                    elif value.startswith('"') and value.endswith('"'):
                        value = value[1:-1]  # Remove quotes
                    elif value.startswith("'") and value.endswith("'"):
                        value = value[1:-1]  # Remove quotes
                except:
                    value = value  # Keep as string
                
                return {
                    "field": field,
                    "operator": op if op != '=' else '==',
                    "value": value
                }
        
        # Default to equality if no operator found
        return {
            "field": condition,
            "operator": "==",
            "value": True  # Field exists
        }
    
    @staticmethod
    def parse_query_params(params: Dict[str, str]) -> Dict[str, Any]:
        """Parse all query parameters into a query object"""
        query = {
            "where": [],
            "order_by": None,
            "order_direction": "asc",
            "limit": None,
            "start_after": None
        }
        
        for key, value in params.items():
            if key == "where":
                # Support multiple where conditions
                if isinstance(value, str):
                    query["where"].append(QueryParser.parse_where_condition(value))
                elif isinstance(value, list):
                    for condition in value:
                        query["where"].append(QueryParser.parse_where_condition(condition))
            
            elif key == "orderBy":
                query["order_by"] = value
            
            elif key == "limit":
                try:
                    query["limit"] = int(value)
                except:
                    query["limit"] = None
            
            elif key == "startAfter":
                query["start_after"] = value
        
        return query

class QueryEngine:
    """Execute queries on data"""
    
    # Operator mappings
    OPERATORS = {
        '==': operator.eq,
        '!=': operator.ne,
        '>': operator.gt,
        '>=': operator.ge,
        '<': operator.lt,
        '<=': operator.le
    }
    
    @staticmethod
    def apply_where(data: List[Dict], conditions: List[Dict]) -> List[Dict]:
        """Apply where conditions to filter data"""
        if not conditions:
            return data
        
        filtered_data = []
        for item in data:
            matches_all = True
            
            for condition in conditions:
                field = condition["field"]
                op_func = QueryEngine.OPERATORS.get(condition["operator"])
                value = condition["value"]
                
                # Get field value from nested data
                field_value = QueryEngine._get_nested_value(item["data"], field)
                
                # Check if field exists for existence checks
                if condition["operator"] == "==" and value is True:
                    if field_value is None:
                        matches_all = False
                        break
                    continue
                
                # Apply operator
                if field_value is None or op_func is None or not op_func(field_value, value):
                    matches_all = False
                    break
            
            if matches_all:
                filtered_data.append(item)
        
        return filtered_data
    
    @staticmethod
    def apply_order_by(data: List[Dict], field: str, direction: str = "asc") -> List[Dict]:
        """Sort data by field"""
        if not field:
            return data
        
        def get_sort_key(item):
            return QueryEngine._get_nested_value(item["data"], field) or ""
        
        return sorted(data, key=get_sort_key, reverse=(direction == "desc"))
    
    @staticmethod
    def apply_limit(data: List[Dict], limit: int) -> List[Dict]:
        """Apply limit to results"""
        if not limit:
            return data
        return data[:limit]
    
    @staticmethod
    def _get_nested_value(obj: Dict, path: str) -> Any:
        """Get nested value from object using dot notation"""
        keys = path.split('.')
        current = obj
        for key in keys:
            if isinstance(current, dict) and key in current:
                current = current[key]
            else:
                return None
        return current

# Create global instances
query_parser = QueryParser()
query_engine = QueryEngine()