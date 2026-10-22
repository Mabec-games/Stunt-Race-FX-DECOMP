/*
 * save_calls.h: the save data and records routines (save/records.c) as C
 * functions, for the C that calls them. Routines whose flags matter to
 * their callers keep them in *p; d is the direct page register where a
 * routine addresses through it. A routine's other exit registers are its
 * adapter's.
 */
#ifndef STUNTRACEFX_SAVE_CALLS_H
#define STUNTRACEFX_SAVE_CALLS_H

#include <stdint.h>

uint8_t SaveRecordSumA(uint8_t *x, uint8_t *p);
uint8_t SaveRecordSumB(uint8_t *x, uint8_t *p);
uint8_t SaveSumsRecordsB(void);
uint8_t SaveRecordsCheckA(uint8_t bank, uint8_t *p);
uint8_t SaveRecordsCheckB(uint8_t bank, uint8_t *p);
void SaveBlocksCheck(uint8_t bank, uint8_t *p);
uint8_t SaveTablesCheck(uint8_t bank, uint8_t *p);
void SaveBlocksValidate(uint8_t bank, uint8_t *p);
void SaveBlocksFourthBad(uint8_t bank, uint8_t *p);
uint8_t SaveValidate(uint8_t bank);
uint8_t SaveUnlocksCheck(void);
uint8_t RecordsUnpackCourse(uint8_t bank);
uint16_t RecordNameLetters(uint8_t bank, uint16_t letters, uint16_t y);
uint8_t RecordTagCarSet(uint8_t tag);
uint8_t RecordTimeDigitsLarge(uint8_t bank, uint16_t d, uint16_t packed, uint16_t y);
uint8_t RecordTimeDigitsSmall(uint8_t bank, uint16_t d, uint16_t packed, uint16_t y);
uint8_t RecordScoreDigits(uint8_t bank, uint16_t d, uint16_t s, uint16_t y);
uint16_t RecordLetterTile(uint16_t d, uint16_t letter);
uint8_t RecordNameLettersLarge(uint8_t bank, uint16_t d, uint16_t letters, uint16_t y);
uint16_t RecordNameLettersSmall(uint8_t bank, uint16_t letters, uint16_t y);
uint16_t RecordRowDraw(uint8_t bank, uint16_t d, uint16_t sprite);
uint16_t RecordRow1(uint8_t bank, uint16_t d);
uint16_t RecordRow2(uint8_t bank, uint16_t d);
uint16_t RecordRow3(uint8_t bank, uint16_t d);
uint16_t RecordRow1Time0(uint8_t bank, uint16_t d);
uint16_t RecordRow2Time4(uint8_t bank, uint16_t d);
uint16_t RecordRow3Time8(uint8_t bank, uint16_t d);
uint16_t RecordRowsFirstTimes(uint8_t bank, uint16_t d);
void RecordTimeCompare(uint8_t bank, uint16_t x);
void RecordCompareTime0(uint8_t bank);
void RecordCompareTime1(uint8_t bank);
uint16_t RecordsLoadSpecialCourse(uint8_t bank, uint16_t d);
uint16_t RecordsLoadSpecialCourseLong(uint16_t d);
void RecordsLoadCourse(uint8_t bank, uint16_t d);
void RecordsLoadForCourse(uint16_t d);
void RecordsLoadCourseLong(uint16_t d);
uint8_t SpecialRecordsChecksum(uint8_t bank, uint16_t d);
void SpecialRecordInsert(uint8_t bank, uint16_t d, uint16_t t);
uint16_t RecordCarTimes12(uint8_t bank);
uint16_t RecordTagCar0(uint8_t bank);
uint16_t RecordTagCar4(uint8_t bank);
uint16_t RecordTagCar8(uint8_t bank);
uint8_t SaveRecordSumsTimeUp(void);
uint8_t TimeUpRecordUpdate(uint8_t bank);
uint8_t StuntRecordTime(uint8_t bank, uint16_t d);
uint16_t RecordsCharOffsetStunt(uint8_t bank);
uint16_t RecordsCharOffsetAll(uint8_t bank);
uint16_t RecordsEntryLoad(uint8_t bank);
uint16_t RecordsCharTileStunt(uint8_t bank);
uint16_t RecordsCharTileAll(uint8_t bank);
uint16_t RecordsCharDrawStunt(uint8_t bank);
uint16_t RecordsCharDrawAll(uint8_t bank);
uint16_t PreviewRowDrawStunt(void);
uint16_t PreviewRowDrawAll(void);

#endif
