//
// Copyright 2019 ZetaSQL Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef ZETASQL_PUBLIC_FUNCTIONS_DATE_TIME_UTIL_H_
#define ZETASQL_PUBLIC_FUNCTIONS_DATE_TIME_UTIL_H_

#include <string>

#include "google/protobuf/timestamp.pb.h"
#include "google/type/date.pb.h"
#include "zetasql/public/civil_time.h"
#include "zetasql/public/functions/datetime.pb.h"
#include "zetasql/public/proto/type_annotation.pb.h"
#include "absl/base/attributes.h"
#include <cstdint>
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "zetasql/base/status.h"
#include "zetasql/base/statusor.h"

// ZetaSQL dates are represented as an int32_t value, indicating the offset
// in days from the epoch 1970-01-01.  ZetaSQL dates are not timezone aware,
// and do not correspond to any particular 24 hour period.
//
// ZetaSQL timestamps are unix timestamps (scaled to include fractional
// seconds), are stored as an int64_t value, and identify a specific point in
// time as an offset in appropriate <scale> units from the epoch
// 1970-01-01 00:00:00+00.  Valid timestamp <scale> values are seconds(0),
// milliseconds(3), microseconds(6), and nanoseconds(9).
//
// Converting between ZetaSQL timestamps and strings requires timezone
// awareness - the timezone must be known in order to map a std::string
// representation of a timestamp to a specific point in time or vice versa.
// At the ZetaSQL level, the timezone can be embedded in the std::string or can
// be provided via a default or separately specified value.  In this library,
// the caller must always provide the relevant time zone to an operation
// when one is required (this library does not have its own default time zone).

namespace zetasql {
namespace functions {

// Returns the SQL fragment corresponding to 'date_part' (which can be used in
// error messages).
absl::string_view DateTimestampPartToSQL(DateTimestampPart date_part);

// Returns the empty std::string if 'date_part' is not a valid DateTimestampPart.
inline absl::string_view DateTimestampPartToSQL(int date_part) {
  return DateTimestampPartToSQL(static_cast<DateTimestampPart>(date_part));
}

// TODO: Move this to datetime.proto.
enum TimestampScale {
  kSeconds = 0,
  kMilliseconds = 3,
  kMicroseconds = 6,
  kNanoseconds = 9,
};

// Checks that a date value falls between 0001-01-01 and 9999-12-31 (inclusive).
ABSL_MUST_USE_RESULT bool IsValidDate(int32_t date);

// Checks that a timestamp value falls between 0001-01-01 and 9999-12-31 UTC
// for timestamps with at most microseconds scale.  For nanoseconds, all int64
// values are considered valid.
ABSL_MUST_USE_RESULT bool IsValidTimestamp(int64_t timestamp,
                                           TimestampScale scale);

// Valid time zone offsets are -14:00 to +14:00, consistent with the SQL
// standard.  The input offset must be at minutes granularity.
ABSL_MUST_USE_RESULT bool IsValidTimeZone(int timezone_minutes_offset);

// Checks that a absl::Time value falls between 0001-01-01 and 9999-12-31 UTC.
ABSL_MUST_USE_RESULT bool IsValidTime(absl::Time time);

// Loads the TimeZone related to the std::string <timezone_string> into <timezone>,
// returning success or failure if <timezone_string> is invalid.  Supports
// a time zone of either the ZetaSQL canonical time zone format ('-08:00')
// or time zone name ('America/Los_Angeles', etc.):
//
//   ((+|-)[D]D[:[D]D]) | (<time zone name>)
//
// Named time zones are loaded from the system's zoneinfo directory (typically
// /usr/share/zoneinfo, /usr/share/lib/zoneinfo, etc.).  As per the base/time
// library, time zone names are case sensitive.
zetasql_base::Status MakeTimeZone(absl::string_view timezone_string,
                          absl::TimeZone* timezone);

// Creates a absl::Time from an int64_t based timestamp value.
ABSL_MUST_USE_RESULT absl::Time MakeTime(int64_t timestamp, TimestampScale scale);

// Converts a absl::Time value into an int64_t timestamp for the given <scale>.
// Returns false if the value is outside the representable range.
ABSL_MUST_USE_RESULT bool FromTime(absl::Time base_time, TimestampScale scale,
                                   int64_t* output);

// Returns a std::string of format "YYYY-MM-DD" for this date.  Returns error
// status if conversion fails.
zetasql_base::Status ConvertDateToString(int32_t date, std::string* out);

// Returns an absl::CivilDay for this date, or an error status if conversion
// fails.
zetasql_base::StatusOr<absl::CivilDay> ConvertDateToCivilDay(int32_t date);

// Populates <out> in canonical timestamp format based on the specified
// <timezone>, for example:
//   "2014-01-31 12:22:34.123456789-08".
// The maximum number of fractional second digits produced is defined by
// <scale>, and the number of fractional digits is a multiple of three
// with trailing zeros truncated if necessary.
// Returns error status if conversion fails.
zetasql_base::Status ConvertTimestampToStringWithTruncation(int64_t timestamp,
                                                    TimestampScale scale,
                                                    absl::TimeZone timezone,
                                                    std::string* out);

// Invokes MakeTimeZone() on <timezone_string> and invokes the prior function.
// Returns error status if <timezone_string> is invalid or conversion fails.
zetasql_base::Status ConvertTimestampToStringWithTruncation(
    int64_t timestamp, TimestampScale scale, absl::string_view timezone_string,
    std::string* out);

// Populates <out> in canonical timestamp format based on the specified
// <timezone>, for example:
//   "2014-01-31 12:22:34.123456789-08".
// The number of fractional second digits produced is defined by <scale>.
// Returns error status if conversion fails.
// Note - these functions do not reflect ZetaSQL semantics for the TIMESTAMP
// type, but they are still being used for the legacy timestamp types.
// They are also useful for debug or test output since the number of
// subsecond digits accurately reflect the timestamp scale.
zetasql_base::Status ConvertTimestampToStringWithoutTruncation(int64_t timestamp,
                                                       TimestampScale scale,
                                                       absl::TimeZone timezone,
                                                       std::string* out);
// Invokes MakeTimeZone() on <timezone_string> and invokes the prior function.
// Returns error status if <timezone_string> is invalid or conversion fails.
zetasql_base::Status ConvertTimestampToStringWithoutTruncation(
    int64_t timestamp, TimestampScale scale, absl::string_view timezone_string,
    std::string* out);

// Populates a absl::Time timestamp into a std::string of canonical format. For
// example: "2014-01-31 12:22:34.123456789-08".
// The maximum number of fractional second digits produced is defined by
// <scale>, and the number of fractional digits is a multiple of three
// with trailing zeros truncated if necessary.
// Returns error status if conversion fails.
zetasql_base::Status ConvertTimestampToString(absl::Time input, TimestampScale scale,
                                      absl::TimeZone timezone, std::string* output);

zetasql_base::Status ConvertTimestampToString(absl::Time input, TimestampScale scale,
                                      absl::string_view timezone_string,
                                      std::string* output);

// Populates a Time into a std::string of canonical format.
// For example: "13:14:15.123456789" for nano precision.
// Use scale to control the maximum number of digits for sub-second. Only
// kMicroSeconds and kNanoSeconds are acceptable. Note that trailing 000's will
// be trimmed.
// Returns error status if conversion fails.
zetasql_base::Status ConvertTimeToString(TimeValue time, TimestampScale scale,
                                 std::string* out);

// Populates a Datetime into a std::string of canonical format.
// For example: "2014-01-31 13:14:15.123456789"
// Use scale to control the maximum number of digits for sub-second. Only
// kMicroSeconds and kNanoSeconds are acceptable. Note that trailing 000's will
// be trimmed.
// Returns error status if conversion fails.
zetasql_base::Status ConvertDatetimeToString(DatetimeValue datetime,
                                     TimestampScale scale, std::string* out);

// Populates <out> using the <format_str> as defined by absl::FormatTime()
// in base/time.h.  Assumes <timestamp> is the number of microseconds from
// 1970-01-01 UTC.
// Returns error status if conversion fails.
//
// The valid format elements are documented in the ZetaSQL function
// specification for the TIMESTAMP_FORMAT() function at:
//
// (broken link)
//
// Note that some format elements are locale-sensitive.  For ZetaSQL
// the locale is en-US.
zetasql_base::Status FormatTimestampToString(absl::string_view format_str,
                                     int64_t timestamp, absl::TimeZone timezone,
                                     std::string* out);

// Invokes MakeTimeZone() on <timezone_string> and invokes the prior function.
// Returns error status if <timezone_string> is invalid or conversion fails.
zetasql_base::Status FormatTimestampToString(absl::string_view format_str,
                                     int64_t timestamp,
                                     absl::string_view timezone_string,
                                     std::string* out);

zetasql_base::Status FormatTimestampToString(absl::string_view format_string,
                                     absl::Time timestamp,
                                     absl::TimeZone timezone, std::string* out);

zetasql_base::Status FormatTimestampToString(absl::string_view format_string,
                                     absl::Time timestamp,
                                     absl::string_view timezone_string,
                                     std::string* out);

// Populates <out> using the <format_string> as defined by absl::FormatTime()
// in base/time.h. Assumes <date> is the number of days from 1970-01-01.
//
// Returns error status if the conversion fails.
zetasql_base::Status FormatDateToString(absl::string_view format_string, int32_t date,
                                std::string* out);

// TODO: Remove after transition.
// Temporary functions to enable smooth transition for supporting quarter
// format qualifier. This function adds <expand_quarter> boolean to control
// whether %Q should be expanded to quarter number or ignored.
zetasql_base::Status FormatTimestampToString(absl::string_view format_str,
                                     absl::Time timestamp,
                                     absl::string_view timezone_string,
                                     bool expand_quarter, std::string* out);

zetasql_base::Status FormatTimestampToString(absl::string_view format_str,
                                     absl::Time timestamp,
                                     absl::TimeZone timezone,
                                     bool expand_quarter, std::string* out);

zetasql_base::Status FormatTimestampToString(absl::string_view format_str,
                                     int64_t timestamp, absl::TimeZone timezone,
                                     bool expand_quarter, std::string* out);

zetasql_base::Status FormatDateToString(absl::string_view format_string, int32_t date,
                                bool expand_quarter, std::string* out);

// Populates <out> using the <format_string> as defined by absl::FormatTime() in
// base/time.h. Returns error status if conversion fails.
//
// The valid format elements are the same as TIMESTAMP_FORMAT() defined in
// (broken link), except that any time zone related
// elements are not allowed.
zetasql_base::Status FormatDatetimeToString(absl::string_view format_string,
                                    const DatetimeValue& datetime, std::string* out);

// Populates <out> using the <format_string> as defined by absl::FormatTime() in
// base/time.h. Returns error status if conversion fails.
//
// The valid format elements are those defined in
// (broken link) that are only related to hours,
// minutes, seconds and subseconds.
zetasql_base::Status FormatTimeToString(absl::string_view format_string,
                                const TimeValue& time, std::string* out);

// Converts the std::string representation of a date to a date value.
// Supported format: "YYYY-[M]M-[D]D".
// Returns error status if conversion fails.
zetasql_base::Status ConvertStringToDate(absl::string_view str, int32_t* date);

// Converts a civil time to a date value, or an error status if conversion
// fails.
zetasql_base::StatusOr<int32_t> ConvertCivilDayToDate(absl::CivilDay civil_day);

// Converts the std::string representation of a timestamp to a timestamp integer
// value of the specified <scale>.  The currently supported format is
// the date format optionally followed by a time and/or time zone
// specified as either of:
//   'YYYY-[M]M-[D]D( |T)[[H]H:[M]M:[S]S[.DDDDDDDDD]][<canonical time zone>]'
//   'YYYY-[M]M-[D]D( |T)[[H]H:[M]M:[S]S[.DDDDDDDDD]][<space> <time zone name>]'
//
// If hours (range 0-23) are specified then minutes (range 0-59) and seconds
// (range 0-59) are required while fractional seconds (1-9 digits) are
// optional.
//
// For example:
//   '2014-01-31'
//   '2014-01-31 -08'
//   '2014-01-31 12:12:34-08'
//   '2014-01-31 12:12:34.1234567-08'
//   '2014-01-31 12:12:34.123456789+08'
//   '2014-01-31T12:12:34.123456789Z'
//   '2014-01-31 12:12:34.123456789'
//   '2014-01-31 12:12:34.123456789 America/Los_Angeles'
//
// If the time zone is not included in the std::string, then <default_timezone> is
// applied for the conversion.
//
// Returns error status if there are more fractional digits than <scale>, or
// conversion otherwise fails.
// DEPRECATED, prefer to use ConvertStringToTimestamp with allow_tz_in_str
// below.
zetasql_base::Status ConvertStringToTimestamp(absl::string_view str,
                                      absl::TimeZone default_timezone,
                                      TimestampScale scale, int64_t* timestamp);

// If the time zone is not included in the std::string, then <default_timezone> is
// applied for the conversion.  Uses the canonical timestamp std::string format.
// If the time zone is included in the std::string and allow_tz_in_str is set to
// false, an error will be returned.
zetasql_base::Status ConvertStringToTimestamp(absl::string_view str,
                                      absl::TimeZone default_timezone,
                                      TimestampScale scale,
                                      bool allow_tz_in_str, int64_t* timestamp);

zetasql_base::Status ConvertStringToTimestamp(absl::string_view str,
                                      absl::TimeZone default_timezone,
                                      TimestampScale scale,
                                      bool allow_tz_in_str, absl::Time* output);

// Converts the std::string representation of a time to a time value of the specified
// <scale>. Returns error status if there are more fractional digits than
// <scale>, or conversion otherwise fails.
// Supported format: "[H]H:[M]M:[S]S[.DDDDDDDDD]".
//
// Note that leap second (:60) is allowed as input, and it will be normalized to
// the first second of the next minute while the sub-second part will be cleared
// if there is any. Comparing to keeping the sub-second, clearing sub-second for
// leap second values:
// 1) introduces less inaccuracy;
// 2) preserves the relative ordering for inputs around the leap seconds.
// For example, for the three value around a leap second:
//   A = 01:02:59.333
//   B = 01:02:60.222
//   C = 01:02:60.444
//   D = 01:03:00.111
// The actual order is A < B < C < D. B and C will be normalized due to the leap
// second.
// If the sub-second part is kept, B' = 01:03:00.222 and C' = 01:03:00.444. The
// order becomes A < D < B' < C'.
// If the sub-second part is cleared, B" = C" = 01:03:00.000. The order becomes
// A < B" = C" < D. Although both B" and C" are collapsed on the same exact leap
// second, their relative order with D are preserved.
zetasql_base::Status ConvertStringToTime(absl::string_view str, TimestampScale scale,
                                 TimeValue* output);

// Converts the std::string representation of a datetime to a datetime value of the
// specified <scale>. Returns error status if there are more fractional digits
// than <scale>, or conversion otherwise fails.
// Supported format: "YYYY-[M]M-[D]D( |T)[[H]H:[M]M:[S]S[.DDDDDDDDD]]"
//
// Same as ConvertStringToTime, leap second (:60) is allowed as input, and it
// will be normalized to the first second of the next minute while the
// sub-second part will be cleared. See the comments on ConvertStringToTime for
// the reasoning of sub-second clearing for input with leap second.
zetasql_base::Status ConvertStringToDatetime(absl::string_view str,
                                     TimestampScale scale,
                                     DatetimeValue* output);

// Invokes MakeTimeZone() on <default_timezone_string> and invokes the prior
// function.  Returns error status if <default_timezone_string> is invalid
// or conversion fails.
// DEPRECATED, prefer to use ConvertStringToTimestamp with allow_tz_in_str
// below.
// Only used in compliance tests.
zetasql_base::Status ConvertStringToTimestamp(absl::string_view str,
                                      absl::string_view default_timezone_string,
                                      TimestampScale scale, int64_t* timestamp);

// If the time zone is not included in the std::string, then
// <default_timezone_string> is applied for the conversion.  Uses the
// canonical timestamp std::string format.
// If the time zone is included in the std::string and allow_tz_in_str is set to
// false, an error will be returned.
zetasql_base::Status ConvertStringToTimestamp(absl::string_view str,
                                      absl::string_view default_timezone_string,
                                      TimestampScale scale,
                                      bool allow_tz_in_str, int64_t* timestamp);

// Populate the <output> with the values from input if the values on all fields
// are valid, return error otherwise.
zetasql_base::Status ConstructDate(int year, int month, int day, int32_t* output);

// Populate <output> with the values from the input if the values on all fields
// are valid, return error otherwise. Note that 60 is considered valid for
// <second>, but the result <output> will be the first second of the next
// minute.
zetasql_base::Status ConstructTime(int hour, int minute, int second, TimeValue* output);

// Populate the <output> with the values from the input if the values on all
// fields are valid, return error otherwise. Note that 60 is considered valid
// for <second>, but the result <output> will be the first second of the next
// minute (which could make the final <output> invalid if it goes out-of-range).
zetasql_base::Status ConstructDatetime(int year, int month, int day, int hour,
                               int minute, int second, DatetimeValue* output);

// Populate the <output> with the values from the input if the values on all
// fields are valid, return error otherwise.
zetasql_base::Status ConstructDatetime(int32_t date, const TimeValue& time,
                               DatetimeValue* output);

// Extracts a DateTimestampPart from the given date
// Returns error status if the input date value is invalid, or if the
// requested DateTimestampPart is not applicable to Date.
zetasql_base::Status ExtractFromDate(DateTimestampPart part, int32_t date, int32_t* output);

// Extracts a DateTimestampPart from the given timestamp as of the specified
// <timezone_string>.  Returns error status if the input timestamp or timezone
// is invalid.  Extracting the DATE part from a timestamp effectively converts
// a timestamp to a date.
zetasql_base::Status ExtractFromTimestamp(DateTimestampPart part, int64_t timestamp,
                                  TimestampScale scale, absl::TimeZone timezone,
                                  int32_t* output);

zetasql_base::Status ExtractFromTimestamp(DateTimestampPart part, int64_t timestamp,
                                  TimestampScale scale,
                                  absl::string_view timezone_string,
                                  int32_t* output);

zetasql_base::Status ExtractFromTimestamp(DateTimestampPart part, absl::Time base_time,
                                  absl::TimeZone timezone, int32_t* output);

zetasql_base::Status ExtractFromTimestamp(DateTimestampPart part, absl::Time base_time,
                                  absl::string_view timezone_string,
                                  int32_t* output);

// Extracts a DateTimestampPart from the given TIME value. Returns error status
// if the input TIME value is invalid.
zetasql_base::Status ExtractFromTime(DateTimestampPart part, const TimeValue& time,
                             int32_t* output);

// Extracts a DateTimestampPart from the given DATETIME value. Returns error
// status if the input DATETIME value is invalid. Extracting the DATE part from
// a DATETIME value effectively converts a DATETIME to a date.
zetasql_base::Status ExtractFromDatetime(DateTimestampPart part,
                                 const DatetimeValue& datetime, int32_t* output);

// Extracts a TIME from the given DATETIME value. Returns error
// status if the input DATETIME value is invalid.
zetasql_base::Status ExtractTimeFromDatetime(const DatetimeValue& datetime,
                                     TimeValue* time);

// Convert a Datetime value to a Timestamp value with the specified timezone.
// Return error status:
// 1. if the input Datetime value is invalid, or
// 2. if the timezone is invalid, or
// 2. if the resulting output Timestamp value is out-of-range.
zetasql_base::Status ConvertDatetimeToTimestamp(const DatetimeValue& datetime,
                                        absl::TimeZone timezone,
                                        absl::Time* output);
zetasql_base::Status ConvertDatetimeToTimestamp(const DatetimeValue& datetime,
                                        absl::string_view timezone_string,
                                        absl::Time* output);

// Convert a Timestamp value to a Datetime value with the specified timezone.
// Returns error status:
// 1. if the input Timestamp value is invalid, or
// 2. if the timezone is invalid, or
// 2. if the resulting output Datetime value is out-of-range.
zetasql_base::Status ConvertTimestampToDatetime(absl::Time base_time,
                                        absl::TimeZone timezone,
                                        DatetimeValue* output);
zetasql_base::Status ConvertTimestampToDatetime(absl::Time base_time,
                                        absl::string_view timezone_string,
                                        DatetimeValue* output);

// Convert a Timestamp value to a Time value with the specified timezone.
// Returns error status if the input timestamp or timezone is invalid.
// Deprecated, use the version with an extra <scale> argument below.
zetasql_base::Status ConvertTimestampToTime(absl::Time base_time,
                                    absl::TimeZone timezone, TimeValue* output);
zetasql_base::Status ConvertTimestampToTime(absl::Time base_time,
                                    absl::TimeZone timezone,
                                    TimestampScale scale, TimeValue* output);
// Deprecated, use the version with an extra <scale> argument below.
zetasql_base::Status ConvertTimestampToTime(absl::Time base_time,
                                    absl::string_view timezone_string,
                                    TimeValue* output);
zetasql_base::Status ConvertTimestampToTime(absl::Time base_time,
                                    absl::string_view timezone_string,
                                    TimestampScale scale, TimeValue* output);

// Converts a Date value to a Timestamp value with the specified timezone.
// Returns error status:
// 1. if the input date value is invalid.
// 2. if the input date value is invalid or if the resulting
// <output> timestamp value does not fit into an int64_t (when <scale> is
// nanoseconds).
zetasql_base::Status ConvertDateToTimestamp(int32_t date, TimestampScale scale,
                                    absl::TimeZone timezone, int64_t* output);

zetasql_base::Status ConvertDateToTimestamp(int32_t date, TimestampScale scale,
                                    absl::string_view timezone_string,
                                    int64_t* output);

// These 2 functions below have same contract as above except the output is
// always valid if the input date is valid.
zetasql_base::Status ConvertDateToTimestamp(int32_t date, absl::TimeZone timezone,
                                    absl::Time* output);

zetasql_base::Status ConvertDateToTimestamp(int32_t date,
                                    absl::string_view timezone_string,
                                    absl::Time* output);

// Converts a Timestamp value to another Timestamp type value.
// Returns error status if the input timestamp value is invalid.
zetasql_base::Status ConvertBetweenTimestamps(int64_t input_timestamp,
                                      TimestampScale input_scale,
                                      TimestampScale output_scale,
                                      int64_t* output);

// Converts a Proto3 Timestamp type (google/protobuf/timestamp.proto) to a
// Timestamp value. The Proto3 Timestamp type is represented with
// nanoseconds precision. Therefore precision loss, in the form of truncation
// towards the past, may occur when converting to the <output_scale>. Returns
// error status if the input timestamp value is invalid.
zetasql_base::Status ConvertProto3TimestampToTimestamp(
    const google::protobuf::Timestamp& input_timestamp,
    TimestampScale output_scale, int64_t* output);
zetasql_base::Status ConvertProto3TimestampToTimestamp(
    const google::protobuf::Timestamp& input_timestamp, absl::Time* output);

// Converts a Timestamp value to a Proto3 Timestamp type. Returns error status
// if the input timestamp value is invalid.
zetasql_base::Status ConvertTimestampToProto3Timestamp(
    int64_t input_timestamp, TimestampScale scale,
    google::protobuf::Timestamp* output);
zetasql_base::Status ConvertTimestampToProto3Timestamp(
    absl::Time input_timestamp, google::protobuf::Timestamp* output);

// Converts a Proto3 Date type (google/type/date.proto) to a Date value. The
// Proto3 Date type supports year and day values of zero, which are not
// currently supported by ZetaSQL Date type. An error status will be returned
// in these cases. An error status is also returned if the input date value is
// invalid.
zetasql_base::Status ConvertProto3DateToDate(const google::type::Date& input,
                                     int32_t* output);

// Converts a Date value to a Proto3 Date type (google/type/date.proto). Returns
// error status if the input date value is invalid.
zetasql_base::Status ConvertDateToProto3Date(int32_t input, google::type::Date* output);

// Add an interval to a part of the given date value.
// Returns error status if the input date value is invalid (out of range), or
// the output date value is invalid (out of range), or the DateTimestampPart
// does not apply to a Date type.
// Allowed DateTimestampPart is YEAR, MONTH, QUARTER, WEEK or DAY.
// A negative interval means a subtraction instead.
zetasql_base::Status AddDate(int32_t date, DateTimestampPart part, int64_t interval,
                     int32_t* output);

// Similar to AddDate, but it doesn't create an error when overflow occurs.
zetasql_base::Status AddDateOverflow(int32_t date, DateTimestampPart part, int32_t interval,
                             int32_t* output, bool* had_overflow);

// Same as above, but subtracts the interval from a part of the date value.
zetasql_base::Status SubDate(int32_t date, DateTimestampPart part, int64_t interval,
                     int32_t* output);

// Returns the diff (signed integer) of the specified DateTimestampPart
// between a given date pair, based on date1 - date2.
// The result DateTimetampPart value of the input dates is not rounded by
// calendar interval, but simply the difference of part values extracted from
// the dates.
// Some examples:
//   DiffDates("2001-12-31", "2001-01-01", YEAR) --> 0
//   DiffDates("2001-01-01", "2000-12-31", YEAR) --> 1
//   DiffDates("2001-02-28", "2001-02-01", MONTH) --> 0
//   DiffDates("2001-02-28", "2000-02-01", MONTH) --> 12
//   DiffDates("2001-02-01", "2000-02-28", MONTH) --> 12
//   DiffDates("2001-02-01", "2001-01-31", MONTH) --> 1
//   DiffDates("2001-02-01", "2002-01-31", MONTH) --> -11
// Returns error status if either of the input date values is invalid (out of
// range), or the DateTimestampPart does not apply to a Date type.
// DateTimestampPart can only be one of YEAR, MONTH, QUARTER, WEEK, WEEK_*, and
// DAY.
zetasql_base::Status DiffDates(int32_t date1, int32_t date2, DateTimestampPart part,
                       int32_t* output);

// Add an interval to a part of the given datetime value.
// Returns error status if the input datetime value is invalid (out of range),
// or the output datetime value is invalid (out of range), or the
// DateTimestampPart does not apply to a Datetime type.
// Allowed DateTimestampPart is YEAR, QUARTER, MONTH, WEEK, DAY, HOUR, MINUTE,
// SECOND, MILLISECOND, MICROSECOND or NANOSECOND.
// A negative interval means a subtraction instead.
zetasql_base::Status AddDatetime(const DatetimeValue& datetime, DateTimestampPart part,
                         int64_t interval, DatetimeValue* output);

// Same as above, but subtracts the interval from a part of the datetime value.
// Prefer this over invoking AddDatetime with a negated interval in order to
// avoid overflow.
zetasql_base::Status SubDatetime(const DatetimeValue& datetime, DateTimestampPart part,
                         int64_t interval, DatetimeValue* output);

// Returns the diff (signed integer) of the specified DateTimestampPart
// between a given datetime pair, based on datetime1 - datetime2.
// The result DateTimetampPart value of the input datetimes is not rounded by
// clock/calendar interval, but simply the difference of part values extracted
// from the datetimes.
// Some examples:
//   DiffDatetimes("2001-12-31", "2001-01-01", YEAR) --> 0
//   DiffDatetimes("2001-01-01", "2000-12-31", YEAR) --> 1
//   DiffDatetimes("2001-02-28", "2001-02-01", MONTH) --> 0
//   DiffDatetimes("2001-02-01", "2001-01-31", MONTH) --> 1
//   DiffDatetimes("2001-02-01 00:59:59", "2001-02-01 00:00:00", HOUR) --> 0
//   DiffDatetimes("2001-02-01 01:00:00", "2001-02-01 00:59:59", HOUR) --> 1
//   DiffDatetimes("2001-02-02 01:00:00", "2001-02-01 00:59:59", HOUR) --> 25
// Returns error status if either of the input datetime values is invalid (out
// of range), or the DateTimestampPart does not apply to a Datetime type, or the
// result overflows an int64_t type (e.g., the difference in nanos between max and
// min nano datetime).
// <part> can only be one of YEAR, QUARTER, MONTH, WEEK, WEEK_*, DAY, HOUR,
// MINUTE, SECOND, MILLISECOND, MICROSECOND and NANOSECOND.
zetasql_base::Status DiffDatetimes(const DatetimeValue& datetime1,
                           const DatetimeValue& datetime2,
                           DateTimestampPart part, int64_t* output);

// Truncates the datetime to the beginning of the specified DateTimestampPart
// granularity.
//
// For example:
//   TruncateDatetime("2016-11-02 12:34:56.456789", YEAR) --> "2016-01-01"
//   TruncateDatetime("2016-11-02 12:34:56.456789", QUARTER) --> "2016-10-01"
//   TruncateDatetime("2016-11-02 12:34:56.456789", DAY) --> "2016-11-02"
//   TruncateDatetime("2016-11-02 12:34:56.456789", MINUTE)
//     --> "2016-11-02 12:34:00"
//   TruncateDatetime("2016-11-02 12:34:56.456789", MILLISECOND)
//     --> "2016-11-02 12:34:56.456"
// Specially, truncating to WEEK returns the beginning of the last Sunday that's
// not later than the datetime.
//   TruncateDatetime("2016-04-20 12:34:56.456789", WEEK) --> "2016-04-17"
// Truncating to WEEK_<WEEKDAY> is similar, returning the beginning of the
// last <WEEKDAY> that's not later than the datetime.
//   TruncateDatetime("2016-04-20 12:34:56.456789", WEEK_MONDAY)
//     --> "2016-04-18"
//
// Allowed DateTimestampParts are YEAR, QUARTER, MONTH, WEEK, WEEK_<WEEKDAY>,
// DAY, HOUR, MINUTE, SECOND, MILLISECOND, MICROSECOND or NANOSECOND.
// Returns error if the datetime is invalid (out of range), or the
// DateTimestampPart is not allowed, or the result is not a valid datetime value
// (e.g. when truncate "0001-01-01 12:34:56" (a Saturday) to WEEK, which suppose
// to return the Sunday before it).
zetasql_base::Status TruncateDatetime(const DatetimeValue& datetime,
                              DateTimestampPart part, DatetimeValue* output);

// Returns the diff (signed integer) of the specified DateTimestampPart
// between a given timestamp pair, based on timestamp1 - timestamp2.
// Returns the number of whole <part> differences between the two
// timestamps (i.e., the number of whole hours, seconds, etc.).
// Returns error status if either the specified date part is unsupported
// (YEAR, DAY, etc.), or the result overflows an int64_t type (i.e., the
// difference in nanos between max and min nano timestamps).  Does not
// range check the arguments.
zetasql_base::Status TimestampDiff(int64_t timestamp1, int64_t timestamp2,
                           TimestampScale scale,
                           DateTimestampPart part, int64_t* output);

// This function has exact same contract as the function above except it takes 2
// absl::Time as input.
zetasql_base::Status TimestampDiff(absl::Time timestamp1, absl::Time timestamp2,
                           DateTimestampPart part, int64_t* output);

// Add an interval to a part of the given Timestamp value.
// Returns error status if the input Timestamp value is invalid (out of range),
// or the interval is invalid (out of range on the DateTimestampPart),
// or the output Timestamp value is invalid (out of range), or the
// DateTimestampPart does not apply to a Timestamp type.
// Allowed DateTimestampPart is DAY, HOUR, MINUTE, SECOND, MILLISECOND,
// MICROSECOND or NANOSECOND.
// DATE, DAYOFWEEK and DAYOFYEAR are not allowed, nor are WEEK, MONTH,
// QUARTER, or YEAR.
// A negative interval means a subtraction instead.
//
// Valid interval ranges are:
// DAY:     [-213300, 213300]            213300 = kDateMax - kDateMin
// HOUR:    [-5119223, 5119223]         5119223 = 213301 * 24 - 1
// MINUTE:  [-307153439, 307153439]   307153439 = 5119224 * 60 - 1
// SECOND:  [-18429206399, 18429206399]
//          18429206399 = kTimestampSecondsMax - kTimestampSecondsMin
// MILLISECOND:
//          [-18429206399999, 18429206399999]
//          18429206399999 = kTimestampMillisMax - kTimestampMillisMin
// MICROSECOND:
//          [-18429206399999999, 18429206399999999]
//          18429206399999999 = kTimestampMicrosMax - kTimestampMicrosMin
// NANOSECOND:
//          [int64min, int64max], because
//          kTimestampNanosMin + kint64max < kTimestampNanosMax
//          kTimestampNanosMax + kint64min > kTimestampNanosMin
//
// Adding a valid non-NANOS interval to an non-NANOS Timestamp value can only
// guarantee there is no arithmetic int64_t overflow. But the result timestamp
// may still be invalid (out of range).
// Adding a NANOSECOND interval to a TIMESTAMP_NANOS value may cause int64
// arithmetic overflow and it will be reported as an error as well.
//
// Note - the timezone is irrelevant for the computations, and is only used
// for error messaging.
zetasql_base::Status AddTimestamp(int64_t timestamp, TimestampScale scale,
                          absl::TimeZone timezone, DateTimestampPart part,
                          int64_t interval, int64_t* output);

zetasql_base::Status AddTimestamp(int64_t timestamp, TimestampScale scale,
                          absl::string_view timezone_string,
                          DateTimestampPart part, int64_t interval,
                          int64_t* output);

// The 2 functions below take absl::Time as input parameter and output
// parameter. We don't need TimestampScale as input parameter.
// The differences between these 2 functions and AddTimestamp() above are
// the following:
// Adding an interval of granularity smaller than a day will not cause
// arithmetic overflow. But it still can cause out of range.
zetasql_base::Status AddTimestamp(absl::Time timestamp, absl::TimeZone timezone,
                          DateTimestampPart part, int64_t interval,
                          absl::Time* output);

zetasql_base::Status AddTimestamp(absl::Time timestamp,
                          absl::string_view timezone_string,
                          DateTimestampPart part, int64_t interval,
                          absl::Time* output);

// Same as above, but subtracts an interval from part of the given Timestamp
// value. Prefer these over invoking AddTimestamp with a negated interval in
// order to avoid overflow.
zetasql_base::Status SubTimestamp(int64_t timestamp, TimestampScale scale,
                          absl::TimeZone timezone, DateTimestampPart part,
                          int64_t interval, int64_t* output);

zetasql_base::Status SubTimestamp(int64_t timestamp, TimestampScale scale,
                          absl::string_view timezone_string,
                          DateTimestampPart part, int64_t interval,
                          int64_t* output);

// Same as the 2 function above except these 2 functions below take absl::Time
// as input parameter and output parameter. We don't need TimestampScale as
// input parameter.
zetasql_base::Status SubTimestamp(absl::Time timestamp, absl::TimeZone timezone,
                          DateTimestampPart part, int64_t interval,
                          absl::Time* output);

zetasql_base::Status SubTimestamp(absl::Time timestamp,
                          absl::string_view timezone_string,
                          DateTimestampPart part, int64_t interval,
                          absl::Time* output);

// Add an interval to a part of the given time value.
// Returns error status if the input time value is invalid (out of range),
// or the DateTimestampPart does not apply to a Time type.
// Note that if addition of the interval results in a TimeValue outside of the
// [00:00:00, 24:00:00) range, the result will be wrapped around to stay within
// 24 hours. For example, adding 2 seconds to 23:59:59 will get 00:00:01.
// Allowed DateTimestampPart is HOUR, MINUTE, SECOND, MILLISECOND, MICROSECOND
// or NANOSECOND.
// A negative interval means a subtraction instead.
zetasql_base::Status AddTime(const TimeValue& time, DateTimestampPart part,
                     int64_t interval, TimeValue* output);

// Same as above, but subtracts the interval from a part of the time value.
// Prefer these over invoking AddTime with a negated interval in order to
// avoid overflow.
zetasql_base::Status SubTime(const TimeValue& time, DateTimestampPart part,
                     int64_t interval, TimeValue* output);

// Returns the diff (signed integer) of the specified DateTimestampPart
// between a given time pair, based on time1 - time2.
// The result DateTimetampPart value of the input times is not rounded by
// clock interval, but simply the difference of part values extracted from the
// time.
// Some examples:
//   DiffTimes("00:59:59", "00:00:00", HOUR) --> 0
//   DiffTimes("01:00:00", "00:59:59", HOUR) --> 1
//   DiffTimes("00:59:59", "01:00:00", HOUR) --> -1
//   DiffTimes("00:59:59", "00:00:00", MINUTE) --> 59
//   DiffTimes("01:59:59", "00:00:00", MINUTE) --> 119
// Returns error status if either of the input time values is invalid (out of
// range), or the DateTimestampPart does not apply to a Time type.
// <part> can only be one of HOUR, MINUTE, SECOND, MILLISECOND,
// MICROSECOND and NANOSECOND.
zetasql_base::Status DiffTimes(const TimeValue& time1, const TimeValue& time2,
                       DateTimestampPart part, int64_t* output);

// Truncates the time to the beginning of the specified DateTimestampPart
// granularity.
// For example:
//   TruncateTime("12:34:56.987654", HOUR) --> "12:00:00"
//   TruncateTime("12:34:56.987654", SECOND) --> "12:34:56"
//   TruncateTime("12:34:56.987654", MILLISECOND) --> "12:34:56.987"
// Allowed DateTimestampPart is HOUR, MINUTE, SECOND, MILLISECOND, MICROSECOND
// and NANOSECOND.
// Returns error if the time is invalid (out of range), or the DateTimestampPart
// is not allowed.
zetasql_base::Status TruncateTime(const TimeValue& time, DateTimestampPart part,
                          TimeValue* output);

// Truncates the date to the beginning of the specified DateTimestampPart
// granularity.
// For example: TruncateDate("2013-12-03", YEAR) --> "2013-01-01".
// Allowed DateTimestampParts are YEAR, QUARTER, MONTH, WEEK, WEEK_<WEEKDAY>,
// and DAY.
// Returns error if the date is invalid (out of range), or the DateTimestampPart
// is not allowed.
zetasql_base::Status TruncateDate(int32_t date, DateTimestampPart part, int32_t* output);

// Truncates the timestamp to the beginning of the specified DateTimestampPart
// granularity.  Assumes that the input <timestamp> is at MICROSECOND precision.
// For example: TIMESTAMP_TRUNC("2013-12-03 12:34:56.987654", SECOND)
//                  --> "2013-12-03 12:34:56"
// Allowed DateTimestampParts are YEAR, QUARTER, MONTH, WEEK, WEEK_<WEEKDAY>,
// DAY, HOUR, MINUTE, SECOND, MILLISECOND, or MICROSECOND (a no-op).
// Returns an error if the timestamp is invalid (out of range), or the
// DateTimestampPart is not allowed (NANOSECOND, DAYOFWEEK, etc.).
//
// This function is independent of time zone for date parts SECOND,
// MILLISECOND, and MICROSECOND.  In other words, doing truncation to these
// parts for any time zone produces the same TIMESTAMP result.
//
// The function result is sensitive to time zone for other supported date
// parts (YEAR to MINUTE).  For those date parts, the timestamp is
// interpreted as of the specified <timezone> and truncation occurs to the
// <part> in that <timezone>.
zetasql_base::Status TimestampTrunc(int64_t timestamp, absl::TimeZone timezone,
                            DateTimestampPart part, int64_t* output);
zetasql_base::Status TimestampTrunc(int64_t timestamp, absl::string_view timezone_string,
                            DateTimestampPart part, int64_t* output);

// The 2 functions below have same contract as the 2 above except the
// following:
// 1. It takes absl::Time as input and output.
// 2. It is up to NANOSECOND precision so truncate a absl::Time at NONASECOND is
// not a noop any more since absl::Time could go up to 1/4 nano second.
//
// It does not share the implementation with the 2 above since we don't know if
// this will introduce performance regression.
zetasql_base::Status TimestampTrunc(absl::Time timestamp, absl::TimeZone timezone,
                            DateTimestampPart part, absl::Time* output);
zetasql_base::Status TimestampTrunc(absl::Time timestamp,
                            absl::string_view timezone_string,
                            DateTimestampPart part, absl::Time* output);

// Truncates the timestamp to the beginning of the specified DateTimestampPart
// granularity.
// For example: TruncateTimestamp("2013-12-03 12:34:56", MINUTE)
//                  --> "2013-12-03 12:34:00"
// Allowed DateTimestampPart is YEAR, QUARTER, MONTH, WEEK, DAY, HOUR, MINUTE,
// SECOND, MILLISECOND, MICROSECOND or NANOSECOND.
// Returns error if the timestamp is invalid (out of range), or the
// DateTimestampPart is not allowed, or the DateTimestampPart does not apply to
// the Timestamp type. (For example, truncating from TIMESTAMP_SECOND to
// MICROSECOND is not supported.)
// DEPRECATED.  Should only be used for legacy timestamp types, as the
// generated error messages are specific to those timestamp types.
// TIMESTAMP_TRUNC() on the new TIMESTAMP type should use TimestampTrunc()
// above instead.
zetasql_base::Status TruncateTimestamp(int64_t timestamp, TimestampScale scale,
                               absl::TimeZone timezone, DateTimestampPart part,
                               int64_t* output);
zetasql_base::Status TruncateTimestamp(int64_t timestamp, TimestampScale scale,
                               absl::string_view timezone_string,
                               DateTimestampPart part, int64_t* output);

// Returns the name of the specified <scale>.
std::string TimestampScale_Name(TimestampScale scale);

// Returns the enum value of a DateTimestampPart
// Returns -1 if the name is not a valid DateTimestampPart enum name.
int DateTimestampPart_FromName(absl::string_view name);

// Decode a date value formatted as <format> into the default zetasql
// DATE format.  <*output_is_null> will be set to true if this encoded value
// should decode as NULL.  (This happens for DATE_DECIMAL format for value 0.)
// Return an error if the format is unsupported or the encoded value is invalid.
zetasql_base::Status DecodeFormattedDate(
    int64_t input_formatted_date, FieldFormat::Format format,
    int32_t* output_date, bool* output_is_null);

// Encode a zetasql DATE value using <format>.
// Return an error if the format is unsupported or the input value cannot
// encoded into that format.
zetasql_base::Status EncodeFormattedDate(
    int32_t input_date, FieldFormat::Format format,
    int32_t* output_formatted_date);

// Returns current date as of the specified timezone.
ABSL_MUST_USE_RESULT int32_t CurrentDate(absl::TimeZone timezone);

// Converts <timezone_string> to absl::TimeZone and invokes previous
// function.  If <timezone_string> is not a valid TimeZone, returns
// an error.
ABSL_MUST_USE_RESULT zetasql_base::Status CurrentDate(absl::string_view timezone_string,
                                              int32_t* date);

// Returns the current timestamp as the number of microseconds from epoch.
ABSL_MUST_USE_RESULT int64_t CurrentTimestamp();

// Given a timestamp as a absl::Time and an initial scale, returns the narrowest
// scale that can still accurately represent the timestamp.
void NarrowTimestampScaleIfPossible(absl::Time time, TimestampScale* scale);

// The namespace 'internal_functions' includes the internal implementation
// details and is not part of the public api.
namespace internal_functions {

zetasql_base::Status ExpandPercentZQ(absl::string_view format_string,
                             absl::Time base_time, absl::TimeZone timezone,
                             bool expand_quarter,
                             std::string* expanded_format_string);

}  // namespace internal_functions
}  // namespace functions
}  // namespace zetasql

#endif  // ZETASQL_PUBLIC_FUNCTIONS_DATE_TIME_UTIL_H_