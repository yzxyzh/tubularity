//**********************************************************
//Copyright 2011 Fethallah Benmansour
//
//Licensed under the Apache License, Version 2.0 (the "License"); 
//you may not use this file except in compliance with the License. 
//You may obtain a copy of the License at
//
//http://www.apache.org/licenses/LICENSE-2.0 
//
//Unless required by applicable law or agreed to in writing, software 
//distributed under the License is distributed on an "AS IS" BASIS, 
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
//See the License for the specific language governing permissions and 
//limitations under the License.
//**********************************************************

#ifndef __itkTubularMetricToPathFilter_h
#define __itkTubularMetricToPathFilter_h

#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include "itksys/SystemTools.hxx"

#include "itkNumericTraits.h"
#include "itkExceptionObject.h"
#include "itkPolyLineParametricTubularPath.h"
#include "itkCommand.h"
#include "itkImageToPathFilter.h"
#include "itkFastMarchingUpwindGradientImageFilter2.h"
#include "itkMultiplyByConstantImageFilter.h"
#include "itkRK4CharacteristicDirectionsToPathFilter.h"

namespace itk
{
	 /** \class TubularMetricToPathFilter
	 *
	 * This filter takes as input a tubularity measure, a start-point and endpoints and produces the
	 * minimal paths linking the start-point to the endpoints.
	 * The tubularity measure is supposed to have strictly positive values.
	 * 
	 * First, the FastMarching filter is called to compute the distance from the start point.
	 * Then new Gradient descent (Range-Kutta) filter is called to perform the gradient descent.
	 *
	 * This filter can be obtimized by adding  a stopping criterion to the Fast Marching filter, 
	 * for instance, itkFastMarchingReachedTargetNodesStoppingCriterion.
	 * That should be done carefully. In fact, the RK4CharacteristicDirectionsToPathFilter uses the estimated
	 * Characteristic directions and interpolates them during the subpixelic descent.
	 * One has to be sure that the used directions are well estimated and that non-estimated ones
	 * (because not reached by the front of the Fast Marching) are not used by the interpolator.
	 * In practice, even if such a scenario is very unlikely to happen, 
	 * we prefere to let the Fast Marching explore the whole domain.
	 * 
	 *
	 *
	 * \author Fethallah Benmansour, fethallah[at]gmail.com
	 *
	 * \ingroup ImageToPathFilters
	 */
	
	template <class TInputImage,
	class TOutputPath = PolyLineParametricTubularPath<TInputImage::ImageDimension> >
	class ITK_EXPORT TubularMetricToPathFilter :
	public ImageToPathFilter< TInputImage, TOutputPath >
	{
	public:
		/** Standard class typedefs. */
		typedef TubularMetricToPathFilter                Self;
		typedef ImageToPathFilter<TInputImage,TOutputPath>					Superclass;
		typedef SmartPointer<Self>																	Pointer;
		typedef SmartPointer<const Self>														ConstPointer;
		
		/** Run-time type information (and related methods). */
		itkTypeMacro( TubularMetricToPathFilter, ImageToPathFilter );
		
		/** Method for creation through the object factory. */
		itkNewMacro(Self);
		
		/** ImageDimension constants */
		itkStaticConstMacro(SetDimension, unsigned int, TInputImage::ImageDimension);
		
		/** Some image typedefs. */
		typedef TInputImage																					InputImageType;
		typedef typename InputImageType::ConstPointer								InputImagePointer;
		typedef typename InputImageType::PixelType									InputImagePixelType;
		typedef typename InputImageType::IndexType									InputImageIndexType;
		typedef typename InputImageType::SpacingType								SpacingType;
		typedef typename InputImageType::PointType								  OriginType;
		
		
		/** Some path typedefs. */
		typedef TOutputPath																					PathType;
		typedef typename PathType::VertexType												VertexType;
		typedef typename PathType::Pointer													PathPointer;
		
		/** Some convenient typedefs. */
		typedef Index< SetDimension >																IndexType;
		typedef ImageRegion< SetDimension >													RegionType;
		typedef Size< SetDimension >																SizeType;
		
		/** Declare the fast marching filter type */
		typedef  itk::FastMarchingUpwindGradientImageFilter2
		< InputImageType, InputImageType >													FastMarchingFilterType;
		typedef typename FastMarchingFilterType::Pointer						FastMarchingFilterPointer;
		
		typedef typename FastMarchingFilterType::NodeContainer			NodeContainerType;
		typedef typename NodeContainerType::Pointer									NodeContainerPointer;
		typedef typename FastMarchingFilterType::NodeType						NodeType;
		typedef typename FastMarchingFilterType::GradientImageType	CharacteristicsImageType;
		typedef typename FastMarchingFilterType::LevelSetImageType	DistanceImageType;
		
		/** Declare Characteristics to path filter  */
		typedef RK4CharacteristicDirectionsToPathFilter
		<CharacteristicsImageType, PathType >												CharacteristicsToPathFilterType;
		typedef typename 
		CharacteristicsToPathFilterType::Pointer										CharacteristicsToPathFilterPointer;
		
		
		
		/** Set the start point. */
		void SetStartPoint(const IndexType& startPointIndex);
		
		/** Clears the list of end points and adds the given point to the list. */
		virtual void SetPathEndPoint( const IndexType & point );
		
		/** Adds the given point to the list. */
		virtual void AddPathEndPoint( const IndexType & index );
		
		/** Sets the end point list. */
		virtual void SetEndPointList( const std::vector<IndexType>& endPointList )
		{
			m_EndPointList = endPointList;
			this->Modified();
		}
		
		/** Get the geodesic distance for the output path at given index. */
		virtual double GetEndPointDistance( unsigned int ind  )
		{
			if( ind >= this->GetNumberOfPathsToExtract() )
			{
				itkExceptionMacro("Path index is out of bounds!");
			}
			
			return m_EndPointDistanceList[ind];
		}
		
		/** Get the output path at an index. */
		virtual PathPointer GetPath( unsigned int ind  )
		{
			if( ind >= this->GetNumberOfPathsToExtract() )
			{
				itkExceptionMacro("Path index is out of bounds!");
			}
			
			return this->GetOutput(ind);
		}
		
		/** Clear the list of end points. */
		virtual void ClearPathEndPoints();
		
		/** 
		 * Set the region of the input image to be processed to speed up 
		 * computation. The start point and the end points must reside in 
		 * this region.
		 */
		virtual void SetRegionToProcess(const RegionType& region);
		
		/**  */
		itkSetMacro(TerminationDistanceFactor, double);
		itkGetMacro(TerminationDistanceFactor, double);
		
		itkSetMacro(DescentStepFactor, double);
		itkGetMacro(DescentStepFactor, double);
		
		itkSetMacro(OscillationFactor, double);
		itkGetMacro(OscillationFactor, double);
		
		itkSetMacro(NbMaxIter, unsigned int);
		itkGetMacro(NbMaxIter, unsigned int);
		
		
	protected:
		TubularMetricToPathFilter();
		~TubularMetricToPathFilter();
		virtual void PrintSelf(std::ostream& os, Indent indent) const;
		
		/** Override since the filter needs all the data for the algorithm */
		void GenerateInputRequestedRegion();
		
		/** Implemention of algorithm */
		void GenerateData(void);
		
		/** Get the arrival function from which to extract the path. */
		virtual unsigned int GetNumberOfPathsToExtract( ) const;
		
		/** Get the next end point from which to back propagate. */
		virtual const IndexType & GetEndPoint( unsigned int );
	private:
		TubularMetricToPathFilter(const Self&); //purposely not implemented
		void operator=(const Self&); //purposely not implemented
		
		void ComputePaths(const InputImageType* input, 
											const CharacteristicsImageType* gradientImage,
											const DistanceImageType* distImage,
											std::vector<PathPointer>& outputPathList,
											std::vector<double>& outputDistanceList);
		
		
		double																		m_TerminationDistanceFactor;
		double																		m_OscillationFactor;
		double																		m_DescentStepFactor;
		unsigned int															m_NbMaxIter;
		
		IndexType																	m_StartPoint;
		bool																			m_IsStartPointGiven;
		std::vector<IndexType>										m_EndPointList;
		std::vector<double>												m_EndPointDistanceList;
		
		RegionType																m_RegionToProcess;
		
	};
	
}

#include "itkTubularMetricToPathFilter.txx"

#endif