/**********************************************************************
 * $Id$
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.refractions.net
 *
 * Copyright (C) 2001-2002 Vivid Solutions Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation. 
 * See the COPYING file for more information.
 *
 **********************************************************************/

#include <geos/geosAlgorithm.h>
#include <geos/util.h>

#define DEBUG 0

#ifndef COMPUTE_Z
#define COMPUTE_Z 1
#endif // COMPUTE_Z

namespace geos {

RobustLineIntersector::RobustLineIntersector(){}
RobustLineIntersector::~RobustLineIntersector(){}

void
RobustLineIntersector::computeIntersection(const Coordinate& p,const Coordinate& p1,const Coordinate& p2)
{
	isProperVar=false;

	// do between check first, since it is faster than the orientation test
	if(Envelope::intersects(p1,p2,p)) {
		if ((CGAlgorithms::orientationIndex(p1,p2,p)==0)&&
			(CGAlgorithms::orientationIndex(p2,p1,p)==0)) {
			isProperVar=true;
			if ((p==p1)||(p==p2)) // 2d only test
			{
				isProperVar=false;
			}
#if COMPUTE_Z
			intPt[0].setCoordinate(p);
			double z = interpolateZ(p, p1, p2);
			if ( z != DoubleNotANumber )
			{
				if ( intPt[0].z == DoubleNotANumber )
					intPt[0].z = z;
				else
					intPt[0].z = (intPt[0].z+z)/2;
			}
#endif // COMPUTE_Z
			result=DO_INTERSECT;
			return;
		}
	}
	result = DONT_INTERSECT;
}

int
RobustLineIntersector::computeIntersect(const Coordinate& p1,const Coordinate& p2,const Coordinate& q1,const Coordinate& q2)
{
#if DEBUG
	cerr<<"RobustLineIntersector::computeIntersect called"<<endl;
	cerr<<" p1:"<<p1.toString()<<" p2:"<<p2.toString()<<" q1:"<<q1.toString()<<" q2:"<<q2.toString()<<endl;
#endif // DEBUG

	isProperVar=false;

	// first try a fast test to see if the envelopes of the lines intersect
	if (!Envelope::intersects(p1,p2,q1,q2))
	{
#if DEBUG
		cerr<<" DONT_INTERSECT"<<endl;
#endif
		return DONT_INTERSECT;
	}

	// for each endpoint, compute which side of the other segment it lies
	// if both endpoints lie on the same side of the other segment,
	// the segments do not intersect
	int Pq1=CGAlgorithms::orientationIndex(p1,p2,q1);
	int Pq2=CGAlgorithms::orientationIndex(p1,p2,q2);

	if ((Pq1>0 && Pq2>0) || (Pq1<0 && Pq2<0)) 
	{
#if DEBUG
		cerr<<" DONT_INTERSECT"<<endl;
#endif
		return DONT_INTERSECT;
	}

	int Qp1=CGAlgorithms::orientationIndex(q1,q2,p1);
	int Qp2=CGAlgorithms::orientationIndex(q1,q2,p2);

	if ((Qp1>0 && Qp2>0)||(Qp1<0 && Qp2<0)) {
#if DEBUG
		cerr<<" DONT_INTERSECT"<<endl;
#endif
		return DONT_INTERSECT;
	}

	bool collinear=Pq1==0 && Pq2==0 && Qp1==0 && Qp2==0;
	if (collinear) {
#if DEBUG
		cerr<<" computingCollinearIntersection"<<endl;
#endif
		return computeCollinearIntersection(p1,p2,q1,q2);
	}

	/*
	 * Check if the intersection is an endpoint.
	 * If it is, copy the endpoint as
	 * the intersection point. Copying the point rather than
	 * computing it ensures the point has the exact value,
	 * which is important for robustness. It is sufficient to
	 * simply check for an endpoint which is on the other line,
	 * since at this point we know that the inputLines must
	 *  intersect.
	 */
	if (Pq1==0 || Pq2==0 || Qp1==0 || Qp2==0) {
#if COMPUTE_Z
		int hits=0;
		double z=0.0;
#endif
		isProperVar=false;
		if (Pq1==0) {
			intPt[0].setCoordinate(q1);
#if COMPUTE_Z
			if ( q1.z != DoubleNotANumber )
			{
				z += q1.z;
				hits++;
			}
#endif
		}
		if (Pq2==0) {
			intPt[0].setCoordinate(q2);
#if COMPUTE_Z
			if ( q2.z != DoubleNotANumber )
			{
				z += q2.z;
				hits++;
			}
#endif
		}
		if (Qp1==0) {
			intPt[0].setCoordinate(p1);
#if COMPUTE_Z
			if ( p1.z != DoubleNotANumber )
			{
				z += p1.z;
				hits++;
			}
#endif
		}
		if (Qp2==0) {
			intPt[0].setCoordinate(p2);
#if COMPUTE_Z
			if ( p2.z != DoubleNotANumber )
			{
				z += p2.z;
				hits++;
			}
#endif
		}
#if COMPUTE_Z
#if DEBUG
		cerr<<"RobustLineIntersector::computeIntersect: z:"<<z<<" hits:"<<hits<<endl;
#endif // DEBUG
		if ( hits ) intPt[0].z = z/hits;
#endif // COMPUTE_Z
	} else {
		isProperVar=true;
		Coordinate *c=intersection(p1, p2, q1, q2);
		intPt[0].setCoordinate(*c);
		delete c;
	}
#if DEBUG
	cerr<<" DO_INTERSECT; intPt[0]:"<<intPt[0].toString()<<endl;
#endif // DEBUG
	return DO_INTERSECT;
}

//bool RobustLineIntersector::intersectsEnvelope(Coordinate& p1,Coordinate& p2,Coordinate& q) {
//	if (((q.x>=min(p1.x,p2.x)) && (q.x<=max(p1.x,p2.x))) &&
//		((q.y>=min(p1.y,p2.y)) && (q.y<=max(p1.y,p2.y)))) {
//			return true;
//	} else {
//		return false;
//	}
//}

int
RobustLineIntersector::computeCollinearIntersection(const Coordinate& p1,const Coordinate& p2,const Coordinate& q1,const Coordinate& q2)
{
#if COMPUTE_Z
	double ztot;
	int hits;
	double p2z;
	double p1z;
	double q1z;
	double q2z;
#endif // COMPUTE_Z

#if DEBUG
	cerr<<"RobustLineIntersector::computeCollinearIntersection called"<<endl;
	cerr<<" p1:"<<p1.toString()<<" p2:"<<p2.toString()<<" q1:"<<q1.toString()<<" q2:"<<q2.toString()<<endl;
#endif // DEBUG

	bool p1q1p2=Envelope::intersects(p1,p2,q1);
	bool p1q2p2=Envelope::intersects(p1,p2,q2);
	bool q1p1q2=Envelope::intersects(q1,q2,p1);
	bool q1p2q2=Envelope::intersects(q1,q2,p2);

	if (p1q1p2 && p1q2p2) {
#if DEBUG
		cerr<<" p1q1p2 && p1q2p2"<<endl;
#endif
		intPt[0].setCoordinate(q1);
#if COMPUTE_Z
		ztot=0;
		hits=0;
		q1z = interpolateZ(q1, p1, p2);
		if (q1z != DoubleNotANumber) { ztot+=q1z; hits++; }
		if (q1.z != DoubleNotANumber) { ztot+=q1.z; hits++; }
		if ( hits ) intPt[0].z = ztot/hits;
#endif
		intPt[1].setCoordinate(q2);
#if COMPUTE_Z
		ztot=0;
		hits=0;
		q2z = interpolateZ(q2, p1, p2);
		if (q2z != DoubleNotANumber) { ztot+=q2z; hits++; }
		if (q2.z != DoubleNotANumber) { ztot+=q2.z; hits++; }
		if ( hits ) intPt[1].z = ztot/hits;
#endif
#if DEBUG
		cerr<<" intPt[0]: "<<intPt[0].toString()<<endl;
		cerr<<" intPt[1]: "<<intPt[1].toString()<<endl;
#endif
		return COLLINEAR;
	}
	if (q1p1q2 && q1p2q2) {
#if DEBUG
		cerr<<" q1p1q2 && q1p2q2"<<endl;
#endif
		intPt[0].setCoordinate(p1);
#if COMPUTE_Z
		ztot=0;
		hits=0;
		p1z = interpolateZ(p1, q1, q2);
		if (p1z != DoubleNotANumber) { ztot+=p1z; hits++; }
		if (p1.z != DoubleNotANumber) { ztot+=p1.z; hits++; }
		if ( hits ) intPt[0].z = ztot/hits;
#endif
		intPt[1].setCoordinate(p2);
#if COMPUTE_Z
		ztot=0;
		hits=0;
		p2z = interpolateZ(p2, q1, q2);
		if (p2z != DoubleNotANumber) { ztot+=p2z; hits++; }
		if (p2.z != DoubleNotANumber) { ztot+=p2.z; hits++; }
		if ( hits ) intPt[1].z = ztot/hits;
#endif
		return COLLINEAR;
	}
	if (p1q1p2 && q1p1q2) {
#if DEBUG
		cerr<<" p1q1p2 && q1p1q2"<<endl;
#endif
		intPt[0].setCoordinate(q1);
#if COMPUTE_Z
		ztot=0;
		hits=0;
		q1z = interpolateZ(q1, p1, p2);
		if (q1z != DoubleNotANumber) { ztot+=q1z; hits++; }
		if (q1.z != DoubleNotANumber) { ztot+=q1.z; hits++; }
		if ( hits ) intPt[0].z = ztot/hits;
#endif
		intPt[1].setCoordinate(p1);
#if COMPUTE_Z
		ztot=0;
		hits=0;
		p1z = interpolateZ(p1, q1, q2);
		if (p1z != DoubleNotANumber) { ztot+=p1z; hits++; }
		if (p1.z != DoubleNotANumber) { ztot+=p1.z; hits++; }
		if ( hits ) intPt[1].z = ztot/hits;
#endif
#if DEBUG
		cerr<<" intPt[0]: "<<intPt[0].toString()<<endl;
		cerr<<" intPt[1]: "<<intPt[1].toString()<<endl;
#endif
		return (q1==p1) && !p1q2p2 && !q1p2q2 ? DO_INTERSECT : COLLINEAR;
	}
	if (p1q1p2 && q1p2q2) {
#if DEBUG
		cerr<<" p1q1p2 && q1p2q2"<<endl;
#endif
		intPt[0].setCoordinate(q1);
#if COMPUTE_Z
		ztot=0;
		hits=0;
		q1z = interpolateZ(q1, p1, p2);
		if (q1z != DoubleNotANumber) { ztot+=q1z; hits++; }
		if (q1.z != DoubleNotANumber) { ztot+=q1.z; hits++; }
		if ( hits ) intPt[0].z = ztot/hits;
#endif
		intPt[1].setCoordinate(p2);
#if COMPUTE_Z
		ztot=0;
		hits=0;
		p2z = interpolateZ(p2, q1, q2);
		if (p2z != DoubleNotANumber) { ztot+=p2z; hits++; }
		if (p2.z != DoubleNotANumber) { ztot+=p2.z; hits++; }
		if ( hits ) intPt[1].z = ztot/hits;
#endif
#if DEBUG
		cerr<<" intPt[0]: "<<intPt[0].toString()<<endl;
		cerr<<" intPt[1]: "<<intPt[1].toString()<<endl;
#endif
		return (q1==p2) && !p1q2p2 && !q1p1q2 ? DO_INTERSECT : COLLINEAR;
	}
	if (p1q2p2 && q1p1q2) {
#if DEBUG
		cerr<<" p1q2p2 && q1p1q2"<<endl;
#endif
		intPt[0].setCoordinate(q2);
#if COMPUTE_Z
		ztot=0;
		hits=0;
		q2z = interpolateZ(q2, p1, p2);
		if (q2z != DoubleNotANumber) { ztot+=q2z; hits++; }
		if (q2.z != DoubleNotANumber) { ztot+=q2.z; hits++; }
		if ( hits ) intPt[0].z = ztot/hits;
#endif
		intPt[1].setCoordinate(p1);
#if COMPUTE_Z
		ztot=0;
		hits=0;
		p1z = interpolateZ(p1, q1, q2);
		if (p1z != DoubleNotANumber) { ztot+=p1z; hits++; }
		if (p1.z != DoubleNotANumber) { ztot+=p1.z; hits++; }
		if ( hits ) intPt[1].z = ztot/hits;
#endif
#if DEBUG
		cerr<<" intPt[0]: "<<intPt[0].toString()<<endl;
		cerr<<" intPt[1]: "<<intPt[1].toString()<<endl;
#endif
		return (q2==p1) && !p1q1p2 && !q1p2q2 ? DO_INTERSECT : COLLINEAR;
	}
	if (p1q2p2 && q1p2q2) {
#if DEBUG
		cerr<<" p1q2p2 && q1p2q2"<<endl;
#endif
		intPt[0].setCoordinate(q2);
#if COMPUTE_Z
		ztot=0;
		hits=0;
		q2z = interpolateZ(q2, p1, p2);
		if (q2z != DoubleNotANumber) { ztot+=q2z; hits++; }
		if (q2.z != DoubleNotANumber) { ztot+=q2.z; hits++; }
		if ( hits ) intPt[0].z = ztot/hits;
#endif
		intPt[1].setCoordinate(p2);
#if COMPUTE_Z
		ztot=0;
		hits=0;
		p2z = interpolateZ(p2, q1, q2);
		if (p2z != DoubleNotANumber) { ztot+=p2z; hits++; }
		if (p2.z != DoubleNotANumber) { ztot+=p2.z; hits++; }
		if ( hits ) intPt[1].z = ztot/hits;
#endif
#if DEBUG
		cerr<<" intPt[0]: "<<intPt[0].toString()<<endl;
		cerr<<" intPt[1]: "<<intPt[1].toString()<<endl;
#endif
		return (q2==p2) && !p1q1p2 && !q1p1q2 ? DO_INTERSECT : COLLINEAR;
	}
	return DONT_INTERSECT;
}

Coordinate*
RobustLineIntersector::intersection(const Coordinate& p1,const Coordinate& p2,const Coordinate& q1,const Coordinate& q2) const
{
	Coordinate *n1=new Coordinate(p1);
	Coordinate *n2=new Coordinate(p2);
	Coordinate *n3=new Coordinate(q1);
	Coordinate *n4=new Coordinate(q2);
	Coordinate *normPt=new Coordinate();
	normalize(n1, n2, n3, n4, normPt);
	Coordinate intPt;
	try {
		Coordinate *h=HCoordinate::intersection(*n1,*n2,*n3,*n4);
		intPt.setCoordinate(*h);
#if COMPUTE_Z
		double ztot = 0;
		double zvals = 0;
		double zp = interpolateZ(intPt, p1, p2);
		double zq = interpolateZ(intPt, q1, q2);
		if ( zp != DoubleNotANumber ) { ztot += zp; zvals++; }
		if ( zq != DoubleNotANumber ) { ztot += zq; zvals++; }
		if ( zvals ) intPt.z = ztot/zvals;
#endif // COMPUTE_Z
		delete h;
	} catch (NotRepresentableException *e) {
		Assert::shouldNeverReachHere("Coordinate for intersection is not calculable"+e->toString());
    }
	intPt.x+=normPt->x;
	intPt.y+=normPt->y;
	if (precisionModel!=NULL) {
		precisionModel->makePrecise(&intPt);
	}
	delete n1;
	delete n2;
	delete n3;
	delete n4;
	delete normPt;
    /**
     * MD - after fairly extensive testing
     * it appears that the computed intPt always lies in the segment envelopes
     */
    //if (! isInSegmentEnvelopes(intPt))
    //    System.out.println("outside segment envelopes: " + intPt);

	return new Coordinate(intPt);
}


void
RobustLineIntersector::normalize(Coordinate *n1,Coordinate *n2,Coordinate *n3,Coordinate *n4,Coordinate *normPt) const
{
	normPt->x=smallestInAbsValue(n1->x,n2->x,n3->x,n4->x);
	normPt->y=smallestInAbsValue(n1->y,n2->y,n3->y,n4->y);
	n1->x-=normPt->x;
	n1->y-=normPt->y;
	n2->x-=normPt->x;
	n2->y-=normPt->y;
	n3->x-=normPt->x;
	n3->y-=normPt->y;
	n4->x-=normPt->x;
	n4->y-=normPt->y;

#if COMPUTE_Z
	normPt->z=smallestInAbsValue(n1->z,n2->z,n3->z,n4->z);
	n1->z-=normPt->z;
	n2->z-=normPt->z;
	n3->z-=normPt->z;
	n4->z-=normPt->z;
#endif
}

double
RobustLineIntersector::smallestInAbsValue(double x1,double x2,double x3,double x4) const
{
	double x=x1;
	double xabs=fabs(x);
	if(fabs(x2)<xabs) {
		x=x2;
		xabs=fabs(x2);
	}
	if(fabs(x3)<xabs) {
		x=x3;
		xabs=fabs(x3);
	}
	if(fabs(x4)<xabs) {
		x=x4;
	}
	return x;
}

/*
 * Test whether a point lies in the envelopes of both input segments.
 * A correctly computed intersection point should return <code>true</code>
 * for this test.
 * Since this test is for debugging purposes only, no attempt is
 * made to optimize the envelope test.
 *
 * @return <code>true</code> if the input point lies within both input
 *	segment envelopes
 */
bool
RobustLineIntersector::isInSegmentEnvelopes(const Coordinate& intPt)
{
	Envelope *env0=new Envelope(inputLines[0][0], inputLines[0][1]);
	Envelope *env1=new Envelope(inputLines[1][0], inputLines[1][1]);
	return env0->contains(intPt) && env1->contains(intPt);
}


} // namespace geos

/**********************************************************************
 * $Log$
 * Revision 1.26  2004/11/23 19:53:06  strk
 * Had LineIntersector compute Z by interpolation.
 *
 * Revision 1.25  2004/11/22 13:02:40  strk
 * Fixed a bug in Collinear intersection Z computation
 *
 * Revision 1.24  2004/11/22 11:34:16  strk
 * Added Z computation for CollinearIntersections
 *
 * Revision 1.23  2004/11/20 15:40:49  strk
 * Added Z computation in point-segment intersection.
 *
 * Revision 1.22  2004/11/17 15:09:08  strk
 * Changed COMPUTE_Z defaults to be more conservative
 *
 * Revision 1.21  2004/11/17 08:41:42  strk
 * Fixed a bug in Z computation and removed debugging output by default.
 *
 * Revision 1.20  2004/11/17 08:13:16  strk
 * Indentation changes.
 * Some Z_COMPUTATION activated by default.
 *
 * Revision 1.19  2004/10/21 22:29:54  strk
 * Indentation changes and some more COMPUTE_Z rules
 *
 * Revision 1.18  2004/10/20 17:32:14  strk
 * Initial approach to 2.5d intersection()
 *
 * Revision 1.17  2004/07/02 13:28:26  strk
 * Fixed all #include lines to reflect headers layout change.
 * Added client application build tips in README.
 *
 * Revision 1.16  2004/03/25 02:23:55  ybychkov
 * All "index/*" packages upgraded to JTS 1.4
 *
 * Revision 1.15  2004/03/17 02:00:33  ybychkov
 * "Algorithm" upgraded to JTS 1.4
 *
 * Revision 1.14  2003/11/07 01:23:42  pramsey
 * Add standard CVS headers licence notices and copyrights to all cpp and h
 * files.
 *
 *
 **********************************************************************/
