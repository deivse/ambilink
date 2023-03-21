import functools


class Memoized:
    """Decorator for caching of function return values"""
    def __init__(self, function):
        self._function = function
        self._cache = {}
        
    def __call__(self, *args):
        if args not in self._cache:
            self._cache[args] = self._function(*args)
        return self._cache[args]
    
    def invalidate_cache(self):
        """Resets the memization cache"""
        self._cache = {}

    # Implementing __get__ makes this a descriptor
    def __get__(self, obj, objtype=None):
        if obj is not None:
            # the call is made on an instance, we can pass obj as the self of the function that will be called
            return functools.partial(self.__call__, obj)
        # Called on a class or a raw function, just return self so we can register more callbacks
        return self
