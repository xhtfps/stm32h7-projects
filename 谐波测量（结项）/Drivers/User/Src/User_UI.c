
#include "User_UI.h"

// 更新界面
 void User_UpdataUI( uint8_t ui_name )
{
	LCD_SetTextFont(&CH_Font32);				// 设置3232中文字体,ASCII字体对应为3216
	LCD_SetColor(LCD_WHITE);					//	设置画笔色，使用自定义颜色
    if( ui_name != UI_NULL )
		{
            LCD_SetBackColor(LCD_BLACK); 
			LCD_Clear();
		}
		

    if( ui_name == UI_START )
    {
        LCD_DisplayText(280, 5,"三相高精度功率分析仪");

        LCD_DisplayText(300, 100,"KeyA -> 测量");
        
        LCD_DisplayText(300, 150,"KeyB -> 校准");
        
//        LCD_DisplayText(300, 200,"KeyC -> 题三");
//        
//        LCD_DisplayText(300, 250,"KeyD -> 题四");
        
    }
    else if( ui_name == UI_LOWPOW )
    {

    }
    else if( ui_name == UI_TA_MENU )
    {
        LCD_DisplayText(300, 5,"测量" );
    }
    else if( ui_name == UI_TB_MENU )
    {  
        LCD_DisplayText(300, 5,"校准" );
    }
	else if( ui_name == UI_TC_MENU )
    {  
        LCD_DisplayText(300, 5,"< 题三 >" );
    }
    else if( ui_name == UI_TD_MENU )
    {  
        LCD_DisplayText(300, 5,"< 题四 >" );
    }
}
