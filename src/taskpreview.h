/**
 * @file taskpreview.h
 * @author Joe Wingbermuehle
 * @author josejp2424
 * @date 2026
 *
 * @brief Essora task thumbnail previews.
 *
 * Agregado por josejp2424: preview/thumbnail interno para TaskList.
 */

#ifndef TASKPREVIEW_H
#define TASKPREVIEW_H

#include "client.h"

void InitializeTaskPreview(void);
void ShutdownTaskPreview(void);
void DestroyTaskPreview(void);
void UpdateTaskPreviewCache(ClientNode *np);
void ShowTaskPreview(ClientNode *np, int x, int y);
void HideTaskPreview(void);

#endif
