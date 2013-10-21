/*==============================================================================
Copyright (c) 2012-2013 QUALCOMM Austria Research Center GmbH.
All Rights Reserved.

@file
    BookOverlayView.java

@brief
    Custom View to display the book overlay data

==============================================================================*/
package com.android.baseaugmentation.view;

import com.android.baseaugmentation.R;

import android.content.Context;
import android.graphics.Bitmap;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.widget.ImageView;
import android.widget.RatingBar;
import android.widget.RelativeLayout;
import android.widget.TextView;

/** Custom View with Book Overlay Data */
public class CommentOverlayView extends RelativeLayout
{
    public CommentOverlayView(Context context)
    {
        this(context, null);
    }


    public CommentOverlayView(Context context, AttributeSet attrs)
    {
        this(context, attrs, 0);
    }


    public CommentOverlayView(Context context, AttributeSet attrs, int defStyle)
    {
        super(context, attrs, defStyle);
        inflateLayout(context);

    }


    /** Inflates the Custom View Layout */
    private void inflateLayout(Context context)
    {

        final LayoutInflater inflater = LayoutInflater.from(context);

        // Generates the layout for the view
        inflater.inflate(R.layout.overlay_layout, this, true);
    }


    /** Sets Book title in View */
    public void setTitle(String title)
    {
        TextView tv = (TextView) findViewById(R.id.title);
        tv.setText(title);
    }


    /** Sets Book Author in View */
    public void setDescription(String description)
    {
        TextView tv = (TextView) findViewById(R.id.description);
        tv.setText(description);
    }
    
    public void updateCommentOverlay(String title, String description){
    	this.setTitle(title);
    	this.setDescription(description);
    }
}
