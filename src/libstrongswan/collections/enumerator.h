/*
 * Copyright (C) 2013-2017 Tobias Brunner
 * Copyright (C) 2007 Martin Willi
 * HSR Hochschule fuer Technik Rapperswil
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

/**
 * @defgroup enumerator enumerator
 * @{ @ingroup collections
 */

#ifndef ENUMERATOR_H_
#define ENUMERATOR_H_

typedef struct enumerator_t enumerator_t;

#include <utils/utils.h>

/**
 * Enumerator interface, allows enumeration over collections.
 */
struct enumerator_t {

	/**
	 * Enumerate collection.
	 *
	 * The enumerate function takes a variable argument list containing
	 * pointers where the enumerated values get written.
	 *
	 * @param ...	variable list of enumerated items, implementation dependent
	 * @return		TRUE if pointers returned
	 */
	bool (*enumerate)(enumerator_t *this, ...);

	/**
	 * Destroy a enumerator instance.
	 */
	void (*destroy)(enumerator_t *this);
};

/**
 * Create an enumerator which enumerates over nothing
 *
 * @return			an enumerator over no values
 */
enumerator_t* enumerator_create_empty();

/**
 * Create an enumerator which enumerates over a single item
 *
 * @param item		item to enumerate
 * @param cleanup	cleanup function called on destroy with the item
 * @return			an enumerator over a single value
 */
enumerator_t *enumerator_create_single(void *item, void (*cleanup)(void *item));

/**
 * Create an enumerator over files/subdirectories in a directory.
 *
 * This enumerator_t.enumerate() function returns a (to the directory) relative
 * filename (as a char*), an absolute filename (as a char*) and a file status
 * (to a struct stat), which all may be NULL. "." and ".." entries are
 * skipped.
 *
 * Example:
 *
 * @code
	char *rel, *abs;
	struct stat st;
	enumerator_t *e;

	e = enumerator_create_directory("/tmp");
	if (e)
	{
		while (e->enumerate(e, &rel, &abs, &st))
		{
			if (S_ISDIR(st.st_mode) && *rel != '.')
			{
				printf("%s\n", abs);
			}
		}
		e->destroy(e);
	}
   @endcode
 *
 * @param path		path of the directory
 * @return 			the directory enumerator, NULL on failure
 */
enumerator_t* enumerator_create_directory(const char *path);

/**
 * Create an enumerator over files/directories matching a file pattern.
 *
 * This enumerator_t.enumerate() function returns the filename (as a char*),
 * and a file status (to a struct stat), which both may be NULL.
 *
 * Example:
 *
 * @code
	char *file;
	struct stat st;
	enumerator_t *e;

	e = enumerator_create_glob("/etc/ipsec.*.conf");
	if (e)
	{
		while (e->enumerate(e, &file, &st))
		{
			if (S_ISREG(st.st_mode))
			{
				printf("%s\n", file);
			}
		}
		e->destroy(e);
	}
   @endcode
 *
 * @param pattern	file pattern to match
 * @return 			the enumerator, NULL if not supported
 */
enumerator_t* enumerator_create_glob(const char *pattern);

/**
 * Create an enumerator over tokens of a string.
 *
 * Tokens are separated by one of the characters in sep and trimmed by the
 * characters in trim.
 *
 * @param string	string to parse
 * @param sep		separator characters
 * @param trim		characters to trim from tokens
 * @return			enumerator over char* tokens
 */
enumerator_t* enumerator_create_token(const char *string, const char *sep,
									  const char *trim);

/**
 * Creates an enumerator which enumerates over enumerated enumerators :-).
 *
 * The variable argument list of enumeration values is limit to 5.
 *
 * @param outer					outer enumerator
 * @param inner_constructor		constructor to inner enumerator
 * @param data					data to pass to each inner_constructor call
 * @param destroy_data			destructor to pass to data
 * @return						the nested enumerator
 */
enumerator_t *enumerator_create_nested(enumerator_t *outer,
					enumerator_t *(*inner_constructor)(void *outer, void *data),
					void *data, void (*destroy_data)(void *data));

/**
 * Creates an enumerator which filters output of another enumerator.
 *
 * The filter function receives the user supplied "data" followed by a
 * unfiltered enumeration item, followed by an output pointer where to write
 * the filtered data. Then the next input/output pair follows.
 * It returns TRUE to deliver the
 * values to the caller of enumerate(), FALSE to filter this enumeration.
 *
 * The variable argument list of enumeration values is limit to 5.
 *
 * @param unfiltered			unfiltered enumerator to wrap, gets destroyed
 * @param filter				filter function
 * @param data					user data to supply to filter
 * @param destructor			destructor function to clean up data after use
 * @return						the filtered enumerator
 */
enumerator_t *enumerator_create_filter(enumerator_t *unfiltered,
					bool (*filter)(void *data, ...),
					void *data, void (*destructor)(void *data));

/**
 * Creates an enumerator which filters output of another enumerator.
 *
 * The filter function receives the user supplied "data" followed by the
 * unfiltered enumerator, followed by the arguments passed to the outer
 * enumerator.  It returns TRUE to deliver the values assigned to these
 * arguments to the caller of enumerate(), FALSE to end the enumeration.
 * Actually filtering items is simple as the filter function may just skip
 * enumerated items from the unfiltered enumerator.
 *
 * @param unfiltered			unfiltered enumerator to wrap, gets destroyed
 * @param filter				filter function
 * @param data					user data to supply to filter
 * @param destructor			destructor function to clean up data after use
 * @return						the filtered enumerator
 */
enumerator_t *enumerator_create_filter_new(enumerator_t *unfiltered,
			bool (*filter)(void *data, enumerator_t *unfiltered, va_list list),
			void *data, void (*destructor)(void *data));

/**
 * Defines an enumerator that simplifies filtering/modifying the output of an
 * other enumerator.
 *
 * The output ends with the signature of the outer enumerator's enumerate
 * function (the opening { brace is already written by the macro).
 * The context object is available under the given name. The original enumerator
 * is available in the variable unfiltered.
 *
 * The actual enumerator can be created by calling
 *   enumerator_<name>_create(unfiltered, arg1, void destructor(arg1))
 * where unfiltered is the original enumerator, arg1 is a context object, which
 * may be destroyed with the given destructor (may both be NULL).
 */
#define ENUMERATOR_FILTER(name, arg1_t, arg1_n, ...) \
	typedef struct { \
		enumerator_t public; \
		arg1_t arg1_n; \
		enumerator_t *unfiltered; \
		void (*destructor)(arg1_t); \
	} _##name##_enumerator; \
	METHOD(enumerator_t, _##name##_enumerate, bool, \
		_##name##_enumerator *_this, ...); \
	METHOD(enumerator_t, _##name##_destroy, void, \
		_##name##_enumerator *_this) { \
		if (_this->destructor) { \
			_this->destructor(_this->arg1_n); \
		} \
		_this->unfiltered->destroy(_this->unfiltered); \
		free(_this); \
	} \
	static enumerator_t *enumerator_##name##_create(enumerator_t *unfiltered, \
								arg1_t arg1_n, void (*destructor)(arg1_t)) { \
		_##name##_enumerator *this; \
		INIT(this, \
			.public = { \
				.enumerate = __##name##_enumerate, \
				.destroy = __##name##_destroy, \
			}, \
			.arg1_n = arg1_n, \
			.unfiltered = unfiltered, \
			.destructor = destructor, \
		); \
		return &this->public; \
	} \
	static bool _##name##_enumerate(_##name##_enumerator *_this, ...) { \
		arg1_t arg1_n = _this->arg1_n; \
		enumerator_t *unfiltered = _this->unfiltered; \
		VA_ARGS_GET(_this, __VA_ARGS__);

/**
 * Create an enumerator wrapper which does a cleanup on destroy.
 *
 * @param wrapped				wrapped enumerator
 * @param cleanup				cleanup function called on destroy
 * @param data					user data to supply to cleanup
 * @return						the enumerator with cleanup
 */
enumerator_t *enumerator_create_cleaner(enumerator_t *wrapped,
					void (*cleanup)(void *data), void *data);

#endif /** ENUMERATOR_H_ @}*/
