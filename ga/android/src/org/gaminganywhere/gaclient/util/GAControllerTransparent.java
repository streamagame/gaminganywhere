/*
 * Copyright (c) 2013 Chun-Ying Huang
 *
 * This file is part of GamingAnywhere (GA).
 *
 * GA is free software; you can redistribute it and/or modify it
 * under the terms of the 3-clause BSD License as published by the
 * Free Software Foundation: http://directory.fsf.org/wiki/License:BSD_3Clause
 *
 * GA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the 3-clause BSD License along with GA;
 * if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

package org.gaminganywhere.gaclient.util;

import android.content.Context;
import android.view.MotionEvent;
import android.view.View;

public class GAControllerTransparent extends GAController {
	private float lastX = (float) -1.0;
	private float lastY = (float) -1.0;
	
	public GAControllerTransparent(Context context) {
		super(context);
		this.setMouseVisibility(false);
	}
	
	public static String getName() {
		return "Transparent";
	}
	
	public static String getDescription() {
		return "No Controls! Touch = Click";
	}

	@Override
	public void onDimensionChange(int width, int height) {
		// must be called first
		super.onDimensionChange(width,  height);
		// TODO: initialized and add your controls here
	}
	
	@Override
	public boolean onTouch(View v, MotionEvent evt) {
	
		float x = evt.getX();
		float y = evt.getY();
		
		switch(evt.getActionMasked()) {
		case MotionEvent.ACTION_DOWN:
			lastX = x;
			lastY = y;
			sendMouseMotion(x, y, 0, 0, 0, /*relative=*/false);
			this.sendMouseKey(true, SDL2.Button.LEFT, x, y);
			break;
			
		case MotionEvent.ACTION_UP:
			lastX = x;
			lastY = y;
			sendMouseKey(false, SDL2.Button.LEFT, x, y);
			break;
			
		case MotionEvent.ACTION_MOVE:
				float dx = x-lastX;
				float dy = y-lastY;
				// moveMouse(dx, dy);
				sendMouseMotion(x, y, dx, dy, 0, /*relative=*/false);
				lastX = evt.getX();
				lastY = evt.getX();
			break;
		}
		
		return true; // super.onTouch(v, evt);
	}
}
