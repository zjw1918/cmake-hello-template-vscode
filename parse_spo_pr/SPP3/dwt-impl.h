/*
 * Copyright (c) 2008-2011 Zhang Ming (M. Zhang), zmjerry@163.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 or any later version.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details. A copy of the GNU General Public License is available at:
 * http://www.fsf.org/licensing/licenses
 */


/*****************************************************************************
 *                               dwt-impl.h
 *
 * Implementation for DWT class.
 *
 * Zhang Ming, 2010-03, Xi'an Jiaotong University.
 *****************************************************************************/


/**
 * constructors and destructor
 */
template<typename Type>
DWT<Type>::DWT( const string &wname ) : waveType(wname)
{
    if( waveType != "db4" )
    {
		//LOGE_C("%s,","No such wavelet type!");
        exit(1);
    }

    getFilter(wname);
}

template<typename Type>
DWT<Type>::~DWT()
{
}


/**
 * get coefficients of filter bank
 */
template<typename Type>
inline void DWT<Type>::getFilter( const string &wname )
{
    if( wname == "db4" )
        db4Coefs( ld, hd, lr, hr );
}


/**
 * Initializing the length of coefficients.
 */
template <typename Type>
void DWT<Type>::lengthInit( int sigLength, int J )
{
    lenInfo.resize(J+2);

    int na = sigLength,
        nd = 0,
        total = 0;

    lenInfo[0] = sigLength;
    for( int j=1; j<=J; ++j )
    {
        // detial's length at level j
        nd = (na+hd.dim()-1)/2;

        // approx's length at level j
        na = (na+ld.dim()-1)/2;

        lenInfo[j] = nd;
        total += nd;
    }
    total += na;

    lenInfo[J+1] = na;
}


/**
 * Get the jth level approximation coefficients.
 */
template <typename Type>
Vector<Type> DWT<Type>::getApprox( const Vector<Type> &coefs )
{
    int J = lenInfo.dim()-2;
    Vector<Type> approx(lenInfo[J+1]);

    int start = 0;
    for( int i=1; i<=J; ++i )
        start += lenInfo[i];

    for( int i=0; i<lenInfo[J+1]; ++i )
        approx[i] = coefs[i+start];

    return approx;
}


/**
 * Get the Jth level detial coefficients.
 */
template <typename Type>
Vector<Type> DWT<Type>::getDetial( const Vector<Type> &coefs, int j )
{
	/*
    if( (j < 1) || (j > lenInfo.dim()-2) )
    {
		LOGE_C("%s", "Invalid level for getting detial coefficients!");
        return Vector<Type>(0);
    }
	*/
    Vector<Type> detial(lenInfo[j]);

    int start = 0;
    for( int i=1; i<j; ++i )
        start += lenInfo[i];

    for( int i=0; i<lenInfo[j]; ++i )
        detial[i] = coefs[i+start];

    return detial;
}


/**
 * Set the Jth level approximation coefficients.
 */
template <typename Type>
void DWT<Type>::setApprox( const Vector<Type>&approx, Vector<Type> &coefs )
{
    int J = lenInfo.dim()-2;

    // the approximation's start position in the coe
    int start = 0;
    for( int i=1; i<=J; ++i )
        start += lenInfo[i];

    for( int i=0; i<lenInfo[J+1]; ++i )
        coefs[i+start] = approx[i];
}

template <typename Type>
void DWT<Type>::setApprox(const Type constval, Vector<Type> &coefs)
{
	int J = lenInfo.dim() - 2;

	// the approximation's start position in the coe
	int start = 0;
	for (int i = 1; i <= J; ++i)
		start += lenInfo[i];

	for (int i = 0; i<lenInfo[J + 1]; ++i)
		coefs[i + start] = constval;
}


/**
 * Set the jth level's detial coefficients.
 */
template <typename Type>
void DWT<Type>::setDetial( const Vector<Type> &detial, Vector<Type> &coefs,
                           int j )
{
    // the jth level detial's start position in the coe
    int start = 0;
    for( int i=1; i<j; ++i )
        start += lenInfo[i];

    for( int i=0; i<lenInfo[j]; ++i )
        coefs[i+start] = detial[i];
}

/**
* Set the jth level's detial coefficients.
*/
template <typename Type>
void DWT<Type>::setDetial(const Type constval, Vector<Type> &coefs,
	int j)
{

	// the jth level detial's start position in the coe
	int start = 0;
	for (int i = 1; i<j; ++i)
		start += lenInfo[i];

	for (int i = 0; i<lenInfo[j]; ++i)
		coefs[i + start] = constval;
}

template <typename Type>
Vector<Type> DWT<Type>::reshapesig(const Vector<Type> &signal, int J)
{
	int siglen = signal.size();
	int outlen = siglen + 2 * (J - 1);
	Vector<Type> out(outlen);

	for (int i = J - 2; i >= 0; i--)
	{
		out[J - 2 - i] = signal[i];
	}
	for (int i = 0; i < siglen; i++)
	{
		out[J - 1 + i] = signal[i];
	}
	int tmp = 2 * siglen - 2 + J;
	for (int i = siglen - 1; i > siglen - J; i--)
	{
		out[tmp - i] = signal[i];
	}
	return out;
}

template <typename Type>
Vector<Type> sconv(const Vector<Type> &signal, const Vector<Type> &filter)
{
	int sigLength = signal.size();
	int filLength = filter.size();
	assert(sigLength >= filLength);

	int length = sigLength-filLength+1;
	Vector<Type> x(length);

	for (int i = 0; i < length; i++)
	{
		x[i] = 0;
		for (int j = 0; j < filLength; j++)
			x[i] += filter[j] * signal[filLength + i - j - 1];
	}
	return x;
}

/**
 * Doing J levels decompose for input signal "signal".
 */
template <typename Type>
Vector<Type> DWT<Type>::dwt( const Vector<Type> &signal, int J )
{
    if( (lenInfo.size() != J+2) || (lenInfo[0] != signal.size()) )
    {
        lengthInit( signal.size(), J );
    }
//        lengthInit( signal.size(), J );

    int total = 0;
    for( int i=1; i<=J+1; ++i )
        total += lenInfo[i];

    Vector<Type> coefs(total);
	Vector<Type> approx = signal;
    //Vector<Type> approx = signal;
    Vector<Type> tmp;

    for( int j=1; j<=J; ++j )
    {
		Vector<Type> rsig = reshapesig(approx, hd.dim());
		
		// high frequency filtering
		tmp = sconv(rsig, hd);

		// stored the downsampled signal
        setDetial( dyadDown(tmp,1), coefs, j );

        // low frequency filtering
		tmp = sconv(rsig, ld);
        approx = dyadDown( tmp, 1 );
    }
    setApprox( approx, coefs );

    return coefs;
}


/**
 * Recover the jth level approximation signal.
 * The default parameter is "j=0", which means recover the original signal.
 */
template <typename Type>
Vector<Type> DWT<Type>::idwt( const Vector<Type> &coefs, int j )
{
    if( coefs.size() != (sum(lenInfo)-lenInfo[0]) )
    {
		//LOGE_C("%s","Invalid wavelet coeffcients!");
        return Vector<Type>(0);
    }

    int J = lenInfo.dim() - 2;
    if( (j < 0) || (j > J) )
    {
		//LOGE_C("%s", "Invalid level for reconstructing signal!");
        return Vector<Type>(0);
    }

    Vector<Type> signal = getApprox( coefs );
    Vector<Type> detial;

    for( int i=J; i>j; --i )
    {
        detial = getDetial( coefs, i );

        // upsampling
        signal = dyadUp( signal, 1 );
        detial = dyadUp( detial, 1 );

        // recover the jth approximation
        signal = conv( signal, lr ) + conv( detial, hr );

        // cut off
        signal = wkeep( signal,lenInfo[i-1],"center" );
    }

    return signal;
}
